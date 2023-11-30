#include <read_file.h>

String readFile(fs::FS &fs, const char * path)
{
  String fileContent = "";
  File file = fs.open(path, "r");
  if(!file || file.isDirectory()) return fileContent;
  while(file.available()) fileContent+=String((char)file.read());
  file.close();
  return fileContent;
}