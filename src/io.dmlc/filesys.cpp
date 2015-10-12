#include "filesys.h"

namespace dmlc{
namespace io{

// Return a string without the ending "/".
std::string FileSystem::path(std::string file_name){
  dmlc::io::URI path(file_name.c_str());
  // We don't own the FileSystem pointer.
  dmlc::io::FileSystem *fs = dmlc::io::FileSystem::GetInstance(path.protocol);
  dmlc::io::FileInfo info = fs->GetPathInfo(path);
  if(info.type == dmlc::io::kDirectory){
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
  dmlc::io::URI path(file_name.c_str());
  // We don't own the FileSystem pointer.
  dmlc::io::FileSystem *fs = dmlc::io::FileSystem::GetInstance(path.protocol);
  dmlc::io::FileInfo info = fs->GetPathInfo(path);
  if(info.size == 0){
  	return false;
  }
  return true;
}

bool FileSystem::is_directory(std::string file_name){
  dmlc::io::URI path(file_name.c_str());
  // We don't own the FileSystem pointer.
  dmlc::io::FileSystem *fs = dmlc::io::FileSystem::GetInstance(path.protocol);
  dmlc::io::FileInfo info = fs->GetPathInfo(path);
  if(info.type == dmlc::io::kDirectory){
  	return true;
  }
  return false;
}



} // namespace dmlc::io
} // namespace dmlc
