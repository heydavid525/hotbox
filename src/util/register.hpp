#pragma once

namespace mldb {

// Use RegisterAll to set up all registeries.
void RegisterAll();

void RegisterParsers();

void RegisterCompressors();

void RegisterTransforms();

} // namespace mldb

