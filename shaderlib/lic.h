#define PROD_LUMETOOLS 57
#define PROD_LUMELANDSCAPE 104
#define PROD_LUMELIGHT 197
#define PROD_LUMEWATER 245
#define PROD_LUMEMATTER 365
#define PROD_LUMEWORKBENCH 401
#define PROD_LUMECUSTOM 567

int lic(miState *state, char *software, int prod_id);
void lic_end(int prod_id);
