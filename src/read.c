#include <R.h>
#include <Rinternals.h>
#include <stdio.h>
#include <string.h>

#define STL_HEADER_LEN 80
#define BUFFER_SIZE 256

typedef struct {
  float x, y, z;
} stl_vertex;

typedef struct {
  stl_vertex n, a, b, c;
} stl_triangle;

float* ReadBinary(FILE *stl_src, int verbose_info, int *size) {

  uint32_t facecount;
  uint16_t attribute;

  stl_triangle triangle;

  if (fseek(stl_src, STL_HEADER_LEN, SEEK_SET) != 0) {
    Rf_error("Not able to read header of stl file!");
  }

  if (fread(&facecount, 4, 1, stl_src) != 1) {
    Rf_error("Not able to read number of triangles!");
  }

  float *res = malloc(9*sizeof(float)*facecount);

  if(res == NULL){
    Rf_error("Memory not allocated");
  }

  int vertex = 0;

  for (int f = 0; f < facecount; f++) {

    if (fread(&triangle, 4, 12, stl_src) != 12) {
      return 0;
    }

    res[vertex] = triangle.a.x;
    res[vertex+1] = triangle.a.y;
    res[vertex+2] = triangle.a.z;
    res[vertex+3] = triangle.b.x;
    res[vertex+4] = triangle.b.y;
    res[vertex+5] = triangle.b.z;
    res[vertex+6] = triangle.c.x;
    res[vertex+7] = triangle.c.y;
    res[vertex+8] = triangle.c.z;

    vertex = vertex + 9;

    if (fread(&attribute, 2, 1, stl_src) != 1) {
      return 0;
    }

  }
  * size = (int) vertex;
  return res;
}

static int ReadTriangleASCII(FILE *stl_src, stl_triangle *triangle) {
  if (fscanf(stl_src,
             " facet normal %20f %20f %20f"
             " outer loop"
             " vertex %20f %20f %20f"
             " vertex %20f %20f %20f"
             " vertex %20f %20f %20f"
             " endloop"
             " endfacet ",
             &triangle->n.x, &triangle->n.y, &triangle->n.z,
             &triangle->a.x, &triangle->a.y, &triangle->a.z,
             &triangle->b.x, &triangle->b.y, &triangle->b.z,
             &triangle->c.x, &triangle->c.y, &triangle->c.z) != 12) {
             return 0;
  }
  return 1;
}

float* ReadASCII(FILE *stl_src, int verbose_info, int *size){

  char linebuffer[BUFFER_SIZE];
  char namebuffer[BUFFER_SIZE];

  stl_triangle triangle;

  int triangles = 0;
  int vertex = 0;
  float *res = malloc(9*sizeof(float));

  if(res == NULL){
    Rf_error("Memory not allocated");
  }

  // read first line
  if (fgets(linebuffer, BUFFER_SIZE, stl_src) == NULL) {
    return 0;
  }

  // check for 'solid' keyword
  if (strstr(linebuffer, "solid") == NULL){
    return 0;
  }

  int nameAvailable = sscanf(linebuffer, "solid %1024s", namebuffer);
  if (verbose_info && nameAvailable > 0){
    Rprintf("name: %s \n", namebuffer);
  }

    while (ReadTriangleASCII(stl_src, &triangle) == 1) {
    res = (float*) realloc(res, (9*(vertex+1))*sizeof(float));

    if(res == NULL){
      free(res);
      Rf_error("Memory not allocated");
      return 0;
    }

    res[vertex] = triangle.a.x;
    res[vertex+1] = triangle.a.y;
    res[vertex+2] = triangle.a.z;
    res[vertex+3] = triangle.b.x;
    res[vertex+4] = triangle.b.y;
    res[vertex+5] = triangle.b.z;
    res[vertex+6] = triangle.c.x;
    res[vertex+7] = triangle.c.y;
    res[vertex+8] = triangle.c.z;

    vertex = vertex + 9;
    triangles++;
  }

  if (verbose_info){
    Rprintf("vertices: %d \n", vertex);
    Rprintf("triangles: %d \n", triangles);
  }

  if (fgets(linebuffer, BUFFER_SIZE, stl_src) == NULL) {
    return 0;
  }

  // check for 'endsolid' keyword and identifier
  if(strstr(linebuffer, "endsolid") == NULL){
    return 0;
  }

  *size = vertex;

  return res;

}

SEXP readSTL_(SEXP sFilename, SEXP verbose) {
  const char *fn;
  fn = CHAR(STRING_ELT(sFilename, 0));
  int verbose_info = (asInteger(verbose) == 1);
  float *res;
  int res_size;

  // open file
  FILE *stl_file = fopen(fn, "r");
  if (stl_file == NULL){
    Rf_error("Unable to open file");
    return R_NilValue;
  }

  // first try to read the file as ASCII STL,
  // and check for 'solid' keyword
  res = ReadASCII(stl_file, verbose_info, &res_size);
  if (res == 0){
    rewind(stl_file);
    res  = ReadBinary(stl_file, verbose_info, &res_size);
  }

  fclose(stl_file);

  // convert back to R
  SEXP res_;
  res_ = PROTECT(allocVector(REALSXP, res_size));
  // see: https://github.com/hadley/r-internals/blob/master/vectors.md#get-and-set-values
  double* temp = REAL(res_);

  // convert result of matrix of correct type (by-row)
  int counter = 0;
  for(int y = 0; y < res_size/3; y++){
    for (int x = 0; x < 3; x++){
      temp[y+x*res_size/3] = res[counter];
      counter++;
    }
  }

  // Set dimensions for export to R
  SEXP dim;
  dim = allocVector(INTSXP, 2);
  INTEGER(dim)[0] = res_size/3;
  INTEGER(dim)[1] = 3;
  setAttrib(res_, R_DimSymbol, dim);
  UNPROTECT(1);

  free(res);

  return res_;
}

