echo on

"%VS140COMNTOOLS%..\IDE\Devenv" NFMessage.sln /Rebuild "Release|Any CPU"

.\Precompile\precompile.exe -o:.\XNFMessage.serializer.dll -t:XNFMsgSerializer .\bin\Release\NFMessage.dll


rem xcopy .\bin\Release\*.dll ..\..\..\.\Assets\Plugins\ /s /e /Y

pause