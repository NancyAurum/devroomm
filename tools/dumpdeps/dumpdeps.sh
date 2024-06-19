for obj in obj/*.OBJ; do
  echo "${obj}:"
  python obj-decoder.py "${obj}"                        \
         | grep -o 'date: .* data'                      \
         | sed "s/', data//" | sed "s/, file: b'/   /"  \
         | sed 's/date: /  /' | sed 's/\\\\/\\/g'
  echo ""
done
