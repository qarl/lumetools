#define max3(a,b,c)  (a>b ? (a>c ? a:c) : (b>c ? b:c))
#define min3(a,b,c)  (a<b ? (a<c ? a:c) : (b<c ? b:c))

#define PI 3.14159265358979323846
#define LUME_INF 1000000000

double clamp(double x,double min,double max);

double range_scale(double x,
		   double orig_min, double orig_max,
		   double target_min, double target_max);

double bias(double x, double b);
     
double gain(double x,double g);

double soft_clamp(double x,double max);
