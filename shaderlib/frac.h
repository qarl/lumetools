struct frac
{
  int    octaves;
  double (*coefficient)[];
  double (*frequency)[];
};

struct frac *frac_create(double lacunarity, double h, int octaves);

void frac_destroy(struct frac *frac);

miScalar frac_scalar(struct frac *frac, miVector p);
miScalar frac_scalar_positive(struct frac *frac, miVector p);
