#include "symmetryTable.hpp"
#include <maya/MFnPlugin.h>

static const char* const cmdName = "createSymmetryTable";
static const char* const version = "0.0.4";
static const char* const author = "Michitaka Inoue";

MStatus initializePlugin(MObject mObj)
{
    MStatus status;

    std::string version_str(version);
    std::string compile_date_str(__DATE__);
    std::string compile_time_str(__TIME__);
    std::string ver(version_str + " / " + compile_date_str + " / " + compile_time_str); 

    MFnPlugin fnPlugin(mObj, author, ver.c_str(), "Any");
    
    status = fnPlugin.registerCommand(cmdName, SymmetryTable::creator);
    if (!status) {
        status.perror("registerCommand");
        return status;
    }
    return MS::kSuccess;
}

MStatus uninitializePlugin(MObject mObj)
{
    MStatus status;

    MFnPlugin fnPlugin(mObj);
    status = fnPlugin.deregisterCommand(cmdName);

    if (!status) {
        status.perror("deregisterCommand");
        return status;
    }

    return MS::kSuccess;
}