# get files


arg=$1
if [ "$arg" = "" ];then
    printf "No argument provided\n"
    echo "Usage: bash test.sh [get | post]"

elif [ "$arg" = "get" ];then
    echo "sending 5 requests..."
    for i in $(seq 5); do
        curl --silent -X GET http://localhost:4221/files/test.txt
    done

elif [ "$arg" = "post" ];then
    echo "posting data..."
    curl -X POST http://localhost:4221/files/test.txt -d "test data!"

else
    printf "Invalid argument\n"
fi

