extern miColor red, orange, yellow, green, blue, violet;

double intensity(miColor color);

void convert_rgb_hsv(miScalar r, miScalar g, miScalar b,
		     miScalar *h, miScalar *s, miScalar *v);

void convert_hsv_rgb(miScalar h, miScalar s, miScalar v,
		     miScalar *r, miScalar *g, miScalar *b);
     
     
