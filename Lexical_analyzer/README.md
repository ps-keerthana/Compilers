make clean && make

for i in {1..25}; do
  echo "=== edge$i.c ==="
  ./parser edgecases/edge$i.c
  echo
done

./parser test1.c      # valid program → prints tokens + parsing successful
./parser test2.c      # @ symbol → lexer error
./parser test3.c      # unclosed /* comment → lexer error
./parser test4.c      # all literals and operators

./parser              # invalid number of arguments
./parser fake.c       # no such file exists