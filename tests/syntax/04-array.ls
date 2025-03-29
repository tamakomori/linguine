func main() {
   array1 = [0, 1, 2];
   for (v in array1) {
      print(v);
   }

   array2 = [];
   array2[0] = 0;
   array2[1] = 1;
   array2[2] = 2;
   push(array2, 3);
   for (v in array2) {
      print(v);
   }

   resize(array2, 2);
   for (v in array2) {
      print(v);
   }
}
