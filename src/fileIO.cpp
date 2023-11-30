#include <fileIO.h>

String readFile(fs::FS &fs, const char * path)
{
  String fileContent = "";
  File file = fs.open(path, FILE_READ);
  if(!file || file.isDirectory()) return fileContent;
  while(file.available()) fileContent+=String((char)file.read());
  file.close();
  return fileContent;
}

void wrieFile(fs::FS &fs, const String &path, const String &data){
  File file = fs.open(path.c_str(), FILE_WRITE);
  file.print(data);
  file.close();
}