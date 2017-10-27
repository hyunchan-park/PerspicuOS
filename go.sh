cd nk && make && cd ..
cd FreeBSD9
make buildkernel KERNCONF=NK -j24
make installkernel KERNCONF=NK INSTKERNNAME=NK

