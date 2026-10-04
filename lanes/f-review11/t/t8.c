#include "orc.h"
static int bad=0,n=0;
#define CK(c,m) do{n++; if(!(c)){bad++; printf("FAIL %s\n",m);} }while(0)
int main(void){
  adf_place_t S,w,p7; adf_place_prime(&S,11); adf_place_prime(&p7,7); int v; fmpz_t A,B; fmpz_init_set_si(A,5); fmpz_init_set_si(B,4);
  w=S; v=99; int st=adf_fmpz_jacobi(&v,&w,A,B); CK(st==ADF_DOMAIN&&adf_place_equal(w,S)&&v==99,"jacobi even where");
  adf_fball_t X; adf_fball_init(X); fmpz_t a,h,d; fmpz_init_set_si(a,1);fmpz_init_set_si(h,2);fmpz_init_set_si(d,1); adf_fball_set_fmpz3(X,a,h,d);
  fmpz_set_si(B,5); w=S; v=99; st=adf_fball_jacobi(&v,&w,X,B); CK(st==ADF_NOT_DETERMINED&&adf_place_equal(w,S)&&v==99,"fball jacobi ND where untouched");
  w=S; v=99; st=adf_fball_kronecker(&v,&w,X,B); CK(st==ADF_NOT_DETERMINED&&adf_place_equal(w,S),"fball kron where");
  adf_ucoset_t U; adf_ucoset_init(U); fmpz_t c; fmpz_init_set_si(c,1); fmpz_set_si(h,4); adf_ucoset_set_fmpz2(U,c,h);
  w=S; v=99; st=adf_ucoset_jacobi(&v,&w,U,B); CK(st==ADF_NOT_DETERMINED&&adf_place_equal(w,S)&&v==99,"uc jacobi where");
  adf_place_t p5; adf_place_prime(&p5,5); w=S; v=99; st=adf_ucoset_legendre(&v,&w,U,p5); CK(st==ADF_NOT_DETERMINED&&adf_place_equal(w,p5),"uc legendre where=p");
  /* idele ND where=v, DOMAIN none */
  adf_idele_t I,J; adf_idele_init(I);adf_idele_init(J); arb_t one; arb_init(one); arb_one(one); fmpq_t r; fmpq_init(r); fmpq_set_si(r,3,1);
  fmpz_set_si(c,1); fmpz_set_si(h,5); adf_ucoset_set_fmpz2(U,c,h); adf_idele_set_parts(I,one,r,U); adf_idele_set_parts(J,one,r,U);
  w=S; v=99; st=adf_idele_hilbert_at(&v,&w,I,J,p7); CK(st==ADF_NOT_DETERMINED||st==ADF_OK,"st"); printf("idele p=7 st=%d\n",st);
  fmpq_set_si(r,7,1); adf_idele_set_parts(I,one,r,U); w=S; v=99; st=adf_idele_hilbert_at(&v,&w,I,J,p7); CK(st==ADF_NOT_DETERMINED&&adf_place_equal(w,p7)&&v==99,"idele ND where=v");
  /* real ND where=inf */
  arb_t x,y; arb_init(x);arb_init(y); arb_set_si(x,1); mag_set_ui(arb_radref(x),3); arb_set_si(y,-2); w=S; v=99; st=adf_real_hilbert(&v,&w,x,y); CK(st==ADF_NOT_DETERMINED&&adf_place_is_archimedean(w)&&v==99,"real ND where");
  /* aliasing a=b for lball and idele */
  w=S; v=99; st=adf_idele_hilbert_at(&v,&w,I,I,p7); printf("alias idele st=%d v=%d\n",st,v);
  printf("t8 checks=%d bad=%d\n",n,bad); return bad; }
