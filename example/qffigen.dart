
import 'package:path/path.dart' as Path;
import 'dart:io';

class TransformEntry {
  String cType;
  String nativeType;
  String dartType;
  TransformEntry(this.cType, this.nativeType, this.dartType);
}

final transformEntries = [

  TransformEntry("void", "Void", "void"),

  TransformEntry("bool", "Bool", "bool"),
  TransformEntry("int", "Int", "int"),
  TransformEntry("char", "Char", "int"),

  TransformEntry("uint8_t", "Uint8", "int"),
  TransformEntry("uint16_t", "Uint16", "int"),
  TransformEntry("uint32_t", "Uint32", "int"),
  TransformEntry("uint64_t", "Uint64", "int"),

  TransformEntry("int8_t", "Int8", "int"),
  TransformEntry("int16_t", "Int16", "int"),
  TransformEntry("int32_t", "Int32", "int"),
  TransformEntry("int64_t", "Int64", "int")

];

final String NewLine = '\n';
final String Tab = '  ';

String funcObjectsCode = '';
String loadFuncsCode = '';

TransformEntry transform(String cType) {

  bool pointer = false;

  if(cType.contains('*')) {
    pointer = true;
    cType = cType.replaceAll('*', '');
  }

  TransformEntry? entry;

  for(int i = 0; i < transformEntries.length; i++) {
    if(transformEntries[i].cType == cType) {
      entry = transformEntries[i];
      break;
    }
  }

  if(entry == null) throw "can't transfrom $cType";

  if(pointer) {

    if(cType == "char") {
      entry.dartType = 'Utf8';
      entry.nativeType = 'Utf8';
    }

    entry = TransformEntry('$cType*', 'Pointer<${entry.nativeType}>', 'Pointer<${entry.nativeType}>');

  }

  return entry;

}

void forEachLine(String line) {

  line = line.trim();

  if(!line.startsWith("DllExport")) return;

  List<String> keywords = line.split(RegExp(r'[\s\(\)\,]+'));
  if(keywords.length < 4) return;

  TransformEntry returnType = transform(keywords[1]);
  String funcName = keywords[2];

  int paramsCount = keywords.length - 4;
  if(paramsCount % 2 != 0) return;
  paramsCount ~/= 2;

  funcObjectsCode += '${Tab}late final ${returnType.dartType} Function(';

  String funcDartParams = '';
  String funcNativeParams = '';

  for(int p = 0, i = 3; p < paramsCount; p++) {

    TransformEntry paramType = transform(keywords[i++]);
    String paramName = keywords[i++];

    if(p != 0) {
      funcObjectsCode += ', ';
      funcDartParams += ', ';
      funcNativeParams += ', ';
    }

    funcObjectsCode += '${paramType.dartType} $paramName';
    funcDartParams += paramType.dartType;
    funcNativeParams += paramType.nativeType;

  }

  funcObjectsCode += ') $funcName;$NewLine';

  loadFuncsCode += '$Tab$Tab$funcName = dylib.lookupFunction';
  loadFuncsCode += '<${returnType.nativeType} Function($funcNativeParams), ';
  loadFuncsCode += '${returnType.dartType} Function($funcDartParams)>';
  loadFuncsCode += "('$funcName');$NewLine";

}

void main() {

  String libFileName = "serial_comm.dll";
  String className = "SerialCommLib";
  String inputPath = Path.join(Path.current, 'native', 'SerialComm', 'dllmain.cpp');
  String outputPath = Path.join(Path.current, 'lib', 'src', 'bindings.dart');

  File file = File(inputPath);
  var lines = file.readAsLinesSync();
  lines.forEach(forEachLine);

  String output = '';
  output += NewLine;
  output += "import 'dart:ffi';$NewLine";
  output += "import 'package:ffi/ffi.dart';$NewLine";
  output += "import 'package:path/path.dart' as Path;$NewLine";
  output += NewLine;

  output += 'class $className {$NewLine';
  output += NewLine;

  output += funcObjectsCode;

  output += NewLine;
  output += '$Tab$className() {$NewLine';

  output += NewLine;
  output += "$Tab${Tab}String libPath = Path.join(Path.current, '$libFileName');$NewLine";
  output += '$Tab${Tab}DynamicLibrary dylib = DynamicLibrary.open(libPath);$NewLine';
  
  output += NewLine;
  output += loadFuncsCode;

  output += NewLine;
  output += '$Tab}$NewLine';

  output += NewLine;
  output += '}$NewLine';

  File dartFile = File(outputPath);
  dartFile.writeAsStringSync(output);

}
