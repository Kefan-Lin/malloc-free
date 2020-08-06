for i in {1..50}
do
    echo "Tesing $i th time********************"
        echo "Tesing $i th time********************" >>result_nolock.txt
#   echo "Testing $i thread_test" >> result1.txt
#    ./thread_test >> result1.txt
#    echo "Testing $i thread_test_malloc_free" >> result1.txt
#    ./thread_test_malloc_free >> result1.txt
#    echo "Testing $i thread_test_malloc_free_change_thread" >> result1.txt
#    ./thread_test_malloc_free_change_thread >> result1.txt
    echo "Testing $i thread_test_measurement" >> result_nolock.txt
    ./thread_test_measurement >> result_nolock.txt
done
echo "Done!" >> result_nolock.txt
echo "Done!"
