#ifndef BIP_GENERATOR_H
#define BIP_GENERATOR_H

#include <string>

#include "Semantico.h"

namespace BipGenerator
{
  void reset();
  void registerDeclaration(const Semantico::Variable &variable);
  void registerAssignment(const Semantico::Variable &variable);
  void registerReadStatement(std::size_t position, const std::string &argument);
  void registerPrintStatement(std::size_t position, const std::string &expression);
  std::string render();
  const std::string &lastCode();
  void writeToFile(const std::string &code);
}

#endif
