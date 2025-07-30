#Create file test
cd prueba || { echo "Error: No se pudo cambiar al directorio 'prueba'"; exit 1; }
echo -e "\n\n"

echo "[test] Se crea archivo: test_file.txt "
touch test_file.txt || { echo "Error: No se pudo crear el archivo test_file.txt"; exit 1; }

echo "[test] Deberia resultar 'test_file.txt' al realizar ls"
ls

#Write file test
echo "[test] Escribo contenido en: test_file.txt"
echo "escribiendo palabras" > test_file.txt

echo "[test] Realizo un 'cat test_file.txt'"
salida=$(cat test_file.txt)

if [ "$salida" == "escribiendo palabras" ]; then
  echo "Test exitoso: la salida es correcta."
else
  echo "Test fallido: la salida no es correcta."
  echo "Esperado: 'escribiendo palabras'"
  echo "Obtenido: '$salida'"
  exit 1
fi

#mkdir test
echo "[test] Creo un directorio: dir_test"
mkdir dir_test
echo "[test] Deberia resultar 'dir_test test_file.txt' al realizar ls"
ls

cd dir_test
echo "[test] Creo un archivo dentro de dir_test con contenido: test_file_dos.txt"
echo "dentro de dir_test" > test_file_dos.txt

echo "[test] Realizo un 'cat test_file_dos.txt'"
salida=$(cat test_file_dos.txt)

if [ "$salida" == "dentro de dir_test" ]; then
  echo "Test exitoso: la salida es correcta."
else
  echo "Test fallido: la salida no es correcta."
  echo "Esperado: 'dentro de dir_test'"
  echo "Obtenido: '$salida'"
  exit 1
fi

#readdir test
echo "[test] Deberia resultar 'test_file_dos.txt' al realizar ls"
ls

#stat file test
echo "[test] Se realiza 'stat test_file_dos.txt'"
stat test_file_dos.txt

#remove file test
echo "[test] Elimino el archivo: test_file_dos.txt"
rm test_file_dos.txt
echo "[test] Deberia resultar vacio al realizar ls"
ls

#rmdir test
cd ..
echo "[test] Elimino el dir: dir_test"
rmdir dir_test
echo "[test] Deberia resultar 'test_file.txt' al realizar ls"
ls

echo "[test] Elimino el archivo: test_file.txt"
rm test_file.txt
echo "[test] Deberia resultar vacio al realizar ls"
ls

#finish
cd ..
umount prueba/

echo "[test] Termino"