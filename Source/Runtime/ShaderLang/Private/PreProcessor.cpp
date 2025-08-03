#include "PreProcessor.h"
#define TCPP_IMPLEMENTATION
#include "Assertion.h"
#include "TcppLibrary.h"
#include <fstream>
#include <sstream>
#include <filesystem>

#include "CoreModule.h"

namespace Thunder
{
	using namespace tcpp;
	String PreProcessor::Process(const String& input)
	{
		Lexer lexer(std::make_unique<StringInputStream>(input));

		bool result = true;

		Preprocessor preprocessor(lexer, { [&result](auto&& arg)
		{
			TAssert(false);
		}, [](const std::string& path, bool isSystem)
		{
			String shaderRoot = GFileManager->GetEngineShaderRoot();
			
			std::ifstream file(shaderRoot + path);
			std::stringstream buffer;
			buffer << file.rdbuf();
			std::string raw_text = buffer.str();
			return std::make_unique<StringInputStream>(raw_text);
		},
		true });

		auto handle = preprocessor.Process();
		return Preprocessor::ToString(handle);
	}

}
