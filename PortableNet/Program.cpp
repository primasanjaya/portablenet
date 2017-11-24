//
//  network.cpp
//  PortableNet
//
//  Created by Andrea Vedaldi on 15/10/2017.
//  Copyright © 2017 VGG. All rights reserved.
//

#include "Program.hpp"
#include "json.hpp"

#include <iostream>
#include <fstream>
#include <memory>
#include <cstdio>

using namespace std ;
using namespace nlohmann ;
using namespace vl ;

vl::Context globalContext ;

ofstream Resultfile;

// MARK: - Workspace

void Workspace::print() const
{
  cout << "Workspace:" << endl ;
  for (auto const& entry : tensors) {
    auto const& name = entry.first ;
    auto const& tensor = entry.second ;
    Resultfile.open(name, ios::out | ios::binary);
    cout << "\t" << name << ": [" ;
    for (auto d : tensor.getDimensions()) { cout << d << " " ; }
    cout << "] " ;
    if (tensor.getMemory()) {
      switch (tensor.getDataType()) {
        case VLDT_Double:
          cout << static_cast<double const*>(tensor.getMemory())[0] << endl ;
          Resultfile.write(reinterpret_cast<char const*>(static_cast<double const*>(tensor.getMemory())), (tensor.getNumElements()*sizeof(double)));
          break ;
        case VLDT_Float:
          cout << static_cast<float const*>(tensor.getMemory())[0] << endl ;
          Resultfile.write(reinterpret_cast<char const*>(static_cast<float const*>(tensor.getMemory())), (tensor.getNumElements()*sizeof(float)));
          break ;
        case VLDT_Char:
          cout << static_cast<char const*>(tensor.getMemory())[0] << endl ;
          Resultfile.write(reinterpret_cast<char const*>(static_cast<char const*>(tensor.getMemory())), (tensor.getNumElements()*sizeof(char)));
          break ;
      }
      cout << " ..." ;
    } else {
      cout << "<No Data>" ;
    }
    cout << endl ;
    Resultfile.close();
  }
 
}

/// Retrieve a given tensor from the workspace. If there is no such
/// a tensor, a null tensor is returned instead.
vl::Tensor Workspace::get(string name)
{
  // Search if such a tensor exists.
  auto const& found = tensors.find(name) ;
  if (found != tensors.end()) { return found->second ; }
  return Tensor() ; // Null tensor
}

/// Retrieve a tensor with specified type and shape from the workspace.
/// If no such a tensor is already found in the workspace,
/// a new one is allocated, possibly after deleting a previous tensor
/// of the same name (if any).
vl::Tensor Workspace::get(string name, DataType dt, TensorShape const& shape)
{
  // Search if such a tensor exists.
  auto const& found = tensors.find(name) ;
  if (found != tensors.end()) {
    auto& tensor = found->second ;
    if (tensor.getDataType() == dt && tensor == shape) {
      return tensor ;
    }
  }

  // No matching tensor found; create a new one.
  remove(name) ;
  size_t numBytes = shape.getNumElements() * getDataTypeSizeInBytes(dt) ;
  void* memory = malloc(numBytes) ;
  assert(memory) ;
  if (memory == NULL) {
    // Throw or return null tensor?
    numBytes = 0 ;
  }
  Tensor tensor(shape, dt, VLDT_CPU, memory, numBytes) ;

  // Add back to list.
  tensors[name] = tensor ;
  return tensor ;
}

void Workspace::remove(string name)
{
  // Search if such a tensor exists.
  auto const& found = tensors.find(name) ;
  if (found != tensors.end()) {
    auto& tensor = found->second ;
    free(tensor.getMemory()) ;
    tensors.erase(found) ;
  }
}

bool Workspace::exists(string name) const
{
  auto const& found = tensors.find(name) ;
  return (found != tensors.end()) ;
}

Workspace::~Workspace()
{
  for (auto x = tensors.begin() ; x != tensors.end() ; x = tensors.begin()) {
    remove(x->first) ;
  }
}

std::string const& Workspace::baseName() const
{
  return baseNameString ;
}

void Workspace::baseName(std::string const& name)
{
  baseNameString = name ;
}

// MARK: - Program

vl::ErrorCode Program::execute(Workspace& ws)
{
  for (auto const& op : source["operations"]) {
    auto type = op["type"].get<string>() ;
    cout << "Executing " << type << endl ;
    if (type == "Load") {
      PNCHECK(Load(op, ws)) ;
    }
    else if (type == "dagnn.Conv") {
      PNCHECK(Conv(op, ws)) ;
    }
    else if (type == "dagnn.Pooling") {
      PNCHECK(Pool(op, ws));
    }
    else if (type == "dagnn.ReLU") {
      PNCHECK(ReLU(op, ws));
    }
    else if (type == "LoadImage") {
      PNCHECK(LoadImage(op, ws)) ;
    }
  }
  return VLE_Success ;
}

void Program::load(std::string fileName)
{
  // Open the JSON file describing the neural network.
  auto jsonFileName = fileName + "/net.json" ;
  auto jsonFile = ifstream(jsonFileName) ;
  if (!jsonFile.is_open()) {
    return ;
  }

  // Parse the JSON file.
  source.clear() ;
  jsonFile >> source ;

  // Todo: catch errors.
}

void Program::print() const
{
  cout << source.dump(4) << endl ;
}



