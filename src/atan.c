#include <math.h>

#define PIBY180 0.017453293

int main() {
   int x;
   int y;
   for ( x = -10 ; x < 10 ; x ++ ) for ( y = -10 ; y < 10 ; y ++ ) {
      double a1, a2;
      a1 = atan((-x)/(y?y:0.0001))/PIBY180 + (y<0 ? 180.0 : 0.0);
      a2 = atan2( -x, y ) / PIBY180;
//      if (a2<0) a2 += 360.0;
      if (fabs(a1-a2)>0.05) printf("%d\t%d\t:\t%g\t%g\t%g\n",x,y,a1,a2,a2-a1);
   }
}
