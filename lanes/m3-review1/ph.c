/* Lane m3-review1: adf_phase_get_acb on theta outside [0,1) or not reduced, normal build (MINOR finding). */
#include <adelefeld.h>
#include <stdio.h>
int main(void){ acb_t z; fmpq_t t; slong v[][2]={{5,4},{-3,4},{-1,4},{9,4},{2,4},{5,2},{4,3}}; int i; acb_init(z); fmpq_init(t);
 for(i=0;i<7;i++){ fmpz_set_si(fmpq_numref(t),v[i][0]); fmpz_set_si(fmpq_denref(t),v[i][1]); int st=adf_phase_get_acb(z,t,53);
  printf("theta %ld/%ld st %d ",v[i][0],v[i][1],st); acb_printd(z,6); printf("\n");}
 acb_clear(z); fmpq_clear(t); flint_cleanup_master(); return 0;}
