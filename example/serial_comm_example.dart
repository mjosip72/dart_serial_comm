
import 'package:serial_comm/serial_comm.dart';

void main() {

  SerialPort.onData((String data) {
    print(data);
  });

  SerialPort.listPorts().forEach((SerialPortInfo serialPort) {
    if(serialPort.description.startsWith('USB-SERIAL CH340')) 
      SerialPort.open(portName: serialPort.name, baudRate: 115200);
  });

  SerialPort.onPortClose(() => print('Port closed'));

  Future.delayed(Duration(seconds: 1), () {
    SerialPort.close();
  });

}
