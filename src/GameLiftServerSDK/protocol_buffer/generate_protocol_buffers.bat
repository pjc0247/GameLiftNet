protoc sdk.proto --cpp_out=./
protoc sdk.proto --java_out=./
move /Y sdk.pb.h ./generatedFiles/sdk.pb.h
move /Y sdk.pb.cc ./generatedFiles/sdk.pb.cc