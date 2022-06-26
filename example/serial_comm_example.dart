
import 'package:serial_comm/serial_comm.dart';

void main() {


  SerialPort.listPorts().forEach((e){print(e.description);});

  SerialPort.onData(print);
  SerialPort.onPortClose(() => print('Port closed'));

  SerialPort.open(portName: "COM3", baudRate: 115200);

  Future.delayed(Duration(seconds: 10), () {
    SerialPort.close();
  });

}
