//
// Created by rolf on 1-9-23.
//
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <scip/scip.h>
#include <scip/scipdefplugins.h>
SCIP_RETCODE runSCIP(const std::filesystem::path& inFile){
	SCIP* scip = NULL;
	/*********
	 * Setup *
	 *********/

	/* initialize SCIP */
	SCIP_CALL( SCIPcreate(&scip) );

	SCIPprintVersion(scip,stdout);

	/* include default SCIP plugins */
	SCIP_CALL( SCIPincludeDefaultPlugins(scip) );
	SCIP_CALL(SCIPreadProb(scip,inFile.c_str(),NULL));
	int nVars = SCIPgetNOrigVars(scip);
	SCIP_VAR ** vars = SCIPgetOrigVars(scip);

	for (int i = 0; i < nVars; ++i)
	{
		SCIP_VAR * var = vars[i];
		if(SCIPvarGetType(var) != SCIP_VARTYPE_CONTINUOUS) continue;
		SCIP_Bool infeas = FALSE;
		SCIP_CALL(SCIPchgVarType(scip,var,SCIP_VARTYPE_INTEGER,&infeas));
		assert(!infeas);
	}
	SCIP_CALL(SCIPsolve(scip));
	SCIP_CALL(SCIPfree(&scip));
	return SCIP_OKAY;
}
bool doPresolve(const std::string& problemPath,
		const std::string& presolvedProblemPath,
		const std::string& outputPath){
	if(!std::filesystem::exists(problemPath)){
		std::cerr<<"Input file: "<<problemPath<<" does not exist!\n";
		return false;
	}
	if(!std::filesystem::exists(outputPath)){
		std::cerr<<"Output directory does not yet exist, creating folder at: "<< outputPath <<"\n";
		if(!std::filesystem::create_directory(outputPath)){
			std::cerr<<"Could not create output directory!\n";
			return false;
		}
	}
	auto path = std::filesystem::path(problemPath);
	SCIP_RETCODE code = runSCIP(path);
	return true;
}

int main(int argc, char** argv) {
	std::vector<std::string> args(argv,argv+argc);
	if(args.size() != 4)
	{
		std::cerr<<"Not enough paths specified. Please specify an .mps.gz file, a filename to write the presolved model to and a folder for additional data!\n";
		return EXIT_FAILURE;
	}
	const std::string& fileName = args[1];
	const std::string& presolvedFileName = args[2];
	const std::string& postSolveDirectory = args[3];
	bool good = doPresolve(fileName,presolvedFileName,postSolveDirectory);
	if(!good){
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}