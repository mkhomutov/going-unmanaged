#include <vector>
#include <cstdio>
__attribute__((noinline)) double inner(const std::vector<double>& v){ double s=0; for(double x: v) s+=x*x; return s; }
__attribute__((noinline)) double outer(int n){ std::vector<double> v(n, 1.5); double t=0; for(int i=0;i<2000;++i) t+=inner(v); return t; }
int main(){ double t=0; for(;;){ t+=outer(4096); if(t<0) std::printf("%f\n",t);} }
