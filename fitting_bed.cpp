#include "Marlin.h"
#include "fitting_bed.h"
#ifdef SoftwareAutoLevel




static void Inverse(double *matrix1[],double *matrix2[],int n,double d);
static double Determinant(double* matrix[],int n);
static double AlCo(double* matrix[],int jie,int row,int column);
static double Cofactor(double* matrix[],int jie,int row,int column);

double plainFactorA=0.0, plainFactorB=0.0, plainFactorC=-1.0/ApproachSwitchDistance;
double plainFactorDistance=1.0/ApproachSwitchDistance;
double fittingBedArray[NodeNum][3]={
    {ApproachSwitchHomeOffsetX+approachSwitchOffset[X_AXIS],ApproachSwitchHomeOffsetY+approachSwitchOffset[Y_AXIS],ApproachSwitchDistance},
    {30+approachSwitchOffset[X_AXIS],150+approachSwitchOffset[Y_AXIS],ApproachSwitchDistance},
    {100+approachSwitchOffset[X_AXIS],100+approachSwitchOffset[Y_AXIS],ApproachSwitchDistance},
    {200+approachSwitchOffset[X_AXIS],0+approachSwitchOffset[Y_AXIS],ApproachSwitchDistance},
    {200+approachSwitchOffset[X_AXIS],150+approachSwitchOffset[Y_AXIS],ApproachSwitchDistance},

};


// Ax+By+Cz+1=0


bool fittingBed()
{
    double Y[3];
    plainFactorA = plainFactorB = plainFactorC = 0.0;
    memset(Y, 0, sizeof(Y));
    double *Matrix[3],*IMatrix[3];
    for (int i = 0;i < 3;i++)
    {
        Matrix[i]  = new double[3];
        IMatrix[i] = new double[3];
    }
    for (int i = 0;i < 3;i++)
    {
        for (int j = 0;j < 3;j++)
        {
            *(Matrix[i] + j) = 0.0;
        }
    }
    for (int j = 0;j < 3;j++)
    {
        for (int i = 0;i < NodeNum;i++)
        {
            *(Matrix[0] + j) += fittingBedArray[i][0]*fittingBedArray[i][j];
            *(Matrix[1] + j) += fittingBedArray[i][1]*fittingBedArray[i][j];
            *(Matrix[2] + j) += fittingBedArray[i][2]*fittingBedArray[i][j];
            Y[j] -= fittingBedArray[i][j];
        }
    }
    double d = Determinant(Matrix,3);
    if (fabs(d) < 0.0001)
    {
        SERIAL_ECHOLNPGM("singular matrix");
        
        return -1;
    }
    Inverse(Matrix,IMatrix,3,d);
    for (int i = 0;i < 3;i++)
    {
        plainFactorA += *(IMatrix[0] + i)*Y[i];
        plainFactorB += *(IMatrix[1] + i)*Y[i];
        plainFactorC += *(IMatrix[2] + i)*Y[i];
    }
    plainFactorDistance=sqrt(plainFactorA*plainFactorA+plainFactorB*plainFactorB+plainFactorC*plainFactorC);
    for (int i = 0;i < 3;i++)
    {
        delete[] Matrix[i];
        delete[] IMatrix[i];
    }
    //  getchar();
    return 0;
}

void Inverse(double *matrix1[],double *matrix2[],int n,double d)
{
    int i,j;
    for(i=0;i<n;i++)
        matrix2[i]=(double *)malloc(n*sizeof(double));
    for(i=0;i<n;i++)
        for(j=0;j<n;j++)
            *(matrix2[j]+i)=(AlCo(matrix1,n,i,j)/d);
}

double Determinant(double* matrix[],int n)
{
    double result=0,temp;
    int i;
    if(n==1)
        result=(*matrix[0]);
    else
    {
        for(i=0;i<n;i++)
        {
            temp=AlCo(matrix,n,n-1,i);
            result+=(*(matrix[n-1]+i))*temp;
        }
    }
    return result;
}

double AlCo(double* matrix[],int jie,int row,int column)
{
    double result;
    if((row+column)%2 == 0)
        result = Cofactor(matrix,jie,row,column);
    else result=(-1)*Cofactor(matrix,jie,row,column);
    return result;
}

//int freeRam(void)
//{
//    extern int  __bss_end;
//    extern int  *__brkval;
//    int free_memory;
//    if((int)__brkval == 0) {
//        free_memory = ((int)&free_memory) - ((int)&__bss_end);
//    }
//    else {
//        free_memory = ((int)&free_memory) - ((int)__brkval);
//    }
//    return free_memory;
//}

double Cofactor(double* matrix[],int jie,int row,int column)
{
    double result;
    int i,j;
    double* smallmatr[3];
    for(i=0;i<jie-1;i++)
        smallmatr[i]= new double[jie - 1];
    for(i=0;i<row;i++)
        for(j=0;j<column;j++)
            *(smallmatr[i]+j)=*(matrix[i]+j);
    for(i=row;i<jie-1;i++)
        for(j=0;j<column;j++)
            *(smallmatr[i]+j)=*(matrix[i+1]+j);
    for(i=0;i<row;i++)
        for(j=column;j<jie-1;j++)
            *(smallmatr[i]+j)=*(matrix[i]+j+1);
    for(i=row;i<jie-1;i++)
        for(j=column;j<jie-1;j++)
            *(smallmatr[i]+j)=*(matrix[i+1]+j+1);
    result = Determinant(smallmatr,jie-1);
    
//    SERIAL_ECHOLNPGM("free memory");
//    SERIAL_ECHOLN(freeRam());
    
    
    
    for(i=0;i<jie-1;i++)
        delete[] smallmatr[i];
    return result;
}
#endif