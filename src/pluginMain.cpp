#include "symmetryTable.hpp"
#include <maya/MFnPlugin.h>

static const char* cmdName = "createSymmetryTable";

MStatus initializePlugin(MObject mObj)
{
    MFnPlugin fnPlugin(mObj, "Michitaka Inoue", "0.0.2", "Any");
    fnPlugin.registerCommand(cmdName, SymmetryTable::creator);
    return MS::kSuccess;
}

MStatus uninitializePlugin(MObject mObj)
{
    MFnPlugin fnPlugin(mObj);
    fnPlugin.deregisterCommand(cmdName);
    return MS::kSuccess;
}