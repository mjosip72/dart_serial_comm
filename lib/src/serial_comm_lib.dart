
import 'dart:ffi';
import 'dart:isolate';
import 'dart:typed_data';
import 'package:ffi/ffi.dart';
import 'bindings.dart';

class SerialPortInfo {
  String name;
  String description;
  SerialPortInfo(this.name, this.description);
}

void _reading(SendPort sendPort) {

  SerialCommLib api = SerialCommLib();

  while(true) {

    int bytesToRead = api.waitForEvent();
    if(bytesToRead == -1) break;

    Pointer<Uint8> buffer = malloc.allocate<Uint8>(bytesToRead);

    int bytesRead = api.readData(buffer, bytesToRead);
    if(bytesRead == -1) break;

    Uint8List list = buffer.asTypedList(bytesRead);
    malloc.free(buffer);

    String data = String.fromCharCodes(list);
    sendPort.send(data);

  }

  sendPort.send(null);

}

class SerialPort {

  static bool _portOpen = false;
  static ReceivePort? _receivePort;
  static Isolate? _isolate;

  static void Function(String)? _onData;
  static void Function()? _onPortClose;

  static final SerialCommLib _api = SerialCommLib();

  static List<SerialPortInfo> listPorts() {

    var list = <SerialPortInfo>[];

    int count = _api.listPorts();
    for(int i = 0; i < count; i++) {
      String name = _api.retrievePortName(i).toDartString();
      String description = _api.retrievePortDescription(i).toDartString();
      list.add(SerialPortInfo(name, description));
    }
    _api.clearPortInfo();

    return list;

  }

  static bool open({
    required String portName, required int baudRate, 
    int byteSize = 8, int stopBits = 0, int parity = 0,
    int? rxBuffer, int? txBuffer
    }) {

    if(_portOpen) return false;

    Pointer<Utf8> nativeName = portName.toNativeUtf8();
    _portOpen = _api.openPort(nativeName);
    malloc.free(nativeName);

    if(!_portOpen) return false;

    bool ok = _api.simpleConfigure(baudRate, byteSize, stopBits, parity);
    if(!ok) {
      close();
      return false;
    }

    if(rxBuffer != null && txBuffer != null) {
      ok = _api.configurePortBuffers(rxBuffer, txBuffer);
      if(!ok) {
        close();
        return false;
      }
    }

    _startReading();

    return true;
  }

  static void close() {

    if(!_portOpen) return;

    _receivePort!.close();
    _receivePort = null;

    _portOpen = false;
    _api.closePort();

    _isolate!.kill();
    _isolate = null;

    if(_onPortClose != null) _onPortClose!();
  
  }

  static void _startReading() {

    _receivePort = ReceivePort();
    _receivePort!.listen((dynamic message) {
      if(message is String && _onData != null) _onData!(message);
      else if(message == null) close();
    });

    Isolate.spawn(_reading, _receivePort!.sendPort).then((value) => _isolate = value);

  }

  static void write(String data) {

    if(!_portOpen) return;
    
    Pointer<Utf8> native = data.toNativeUtf8();
    Pointer<Uint8> buffer = native.cast<Uint8>();

    int bytesToWrite = data.length;

    int bytesWritten = 0;
    int remaining = bytesToWrite;

    while(bytesWritten != bytesToWrite) {

      int result = _api.writeData(buffer, bytesWritten, remaining);
      if(result == -1) {
        close();
        break;
      }

      bytesWritten += result;
      remaining -= result;

    }

    malloc.free(buffer);

  }

  static void onData(void Function(String data) onData) => _onData = onData;
  static void onPortClose(void Function() onPortClose) => _onPortClose = onPortClose;
  static bool get isOpen => _portOpen;

}
