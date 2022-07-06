
import 'dart:ffi';
import 'package:ffi/ffi.dart';
import 'package:path/path.dart' as path;

class SerialCommLib {

  late final int Function() listPorts;
  late final Pointer<Utf8> Function(int i) retrievePortName;
  late final Pointer<Utf8> Function(int i) retrievePortDescription;
  late final void Function() clearPortInfo;
  late final bool Function(Pointer<Utf8> portName) openPort;
  late final void Function() closePort;
  late final bool Function(int baudRate, int byteSize, int stopBits, int parity) simpleConfigure;
  late final bool Function(int rxBuffer, int txBuffer) configurePortBuffers;
  late final int Function() waitForEvent;
  late final int Function(Pointer<Uint8> buffer, int count) readData;
  late final int Function(Pointer<Uint8> buffer, int offset, int count) writeData;

  SerialCommLib() {

    String libPath = path.join(path.current, 'serial_comm.dll');
    DynamicLibrary dylib = DynamicLibrary.open(libPath);

    listPorts = dylib.lookupFunction<Int Function(), int Function()>('listPorts');
    retrievePortName = dylib.lookupFunction<Pointer<Utf8> Function(Int), Pointer<Utf8> Function(int)>('retrievePortName');
    retrievePortDescription = dylib.lookupFunction<Pointer<Utf8> Function(Int), Pointer<Utf8> Function(int)>('retrievePortDescription');
    clearPortInfo = dylib.lookupFunction<Void Function(), void Function()>('clearPortInfo');
    openPort = dylib.lookupFunction<Bool Function(Pointer<Utf8>), bool Function(Pointer<Utf8>)>('openPort');
    closePort = dylib.lookupFunction<Void Function(), void Function()>('closePort');
    simpleConfigure = dylib.lookupFunction<Bool Function(Int, Uint8, Uint8, Uint8), bool Function(int, int, int, int)>('simpleConfigure');
    configurePortBuffers = dylib.lookupFunction<Bool Function(Int, Int), bool Function(int, int)>('configurePortBuffers');
    waitForEvent = dylib.lookupFunction<Int64 Function(), int Function()>('waitForEvent');
    readData = dylib.lookupFunction<Int64 Function(Pointer<Uint8>, Uint64), int Function(Pointer<Uint8>, int)>('readData');
    writeData = dylib.lookupFunction<Int64 Function(Pointer<Uint8>, Uint64, Uint64), int Function(Pointer<Uint8>, int, int)>('writeData');

  }

}
