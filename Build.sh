
clear
make -f ./makefile clean
clear
echo '>> compile <<'
echo ' '

make -f ./makefile
make -f ./makefile rmo > /dev/null

rm boost_asio_thread_pool
g++ -O3 boost_asio_thread_pool.cpp -o boost_asio_thread_pool
