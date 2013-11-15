
#ifndef fitting_bed_h
#define fitting_bed_h

#ifdef ApproachSwitch
#define ApproachSwitchDistance  3.0
#define ApproachSwitchHomeOffsetX 30
#define ApproachSwitchHomeOffsetY 0

#endif

#ifdef SoftwareAutoLevel

#define NodeNum 5


extern double plainFactorA, plainFactorB, plainFactorC;
extern double plainFactorDistance;
extern double fittingBedArray[NodeNum][3];

// Ax+By+Cz+1=0

extern bool fittingBed();
//extern void fittingInit();

#endif

#endif
