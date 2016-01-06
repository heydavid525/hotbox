#include "io/filesys.hpp"

namespace dmlc {
namespace io {

// Return a string without the ending "/".
std::string FileSystem::path(std::string file_name){
  URI path(file_name.c_str());
  // We don't own the FileSystem pointer.
  FileSystem *fs = FileSystem::GetInstance(path.protocol);
  FileInfo info = fs->GetPathInfo(path);
  if(info.type == kDirectory){
    return file_name;
  }
  else {
    return parent_path(file_name);
  }
}

std::string FileSystem::parent_path(std::string file_path){
  size_t pos = file_path.rfind("/");
  return file_path.substr(0,pos);
}

bool FileSystem::exist(std::string file_name){
  URI path(file_name.c_str());
  // We don't own the FileSystem pointer.
  FileSystem *fs = FileSystem::GetInstance(path.protocol);
  FileInfo info = fs->GetPathInfo(path);
  if(info.size == 0){
  	return false;
  }
  return true;
}

bool FileSystem::is_directory(std::string file_name){
  URI path(file_name.c_str());
  // We don't own the FileSystem pointer.
  FileSystem *fs = FileSystem::GetInstance(path.protocol);
  FileInfo info = fs->GetPathInfo(path);
  if(info.type == kDirectory){
  	return true;
  }
  return false;
}



} // namespace hotbox::io
} // namespace hotbox
