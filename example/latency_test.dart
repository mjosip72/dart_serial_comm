
import 'package:serial_comm/serial_comm.dart';

int start = 0;

void main() {

  SerialPort.listPorts().forEach((SerialPortInfo serialPort) {
    if(serialPort.description.startsWith('USB-SERIAL CH340')) 
      SerialPort.open(portName: serialPort.name, baudRate: 115200);
  });

  if(!SerialPort.isOpen) return;

  Future.delayed(Duration(seconds: 2), () {

    SerialPort.onData((String data) {
      int end = DateTime.now().microsecondsSinceEpoch;
      print('Latency: ${end - start} us');
      SerialPort.close();
    });

    start = DateTime.now().microsecondsSinceEpoch;
    SerialPort.write('x');

  });

}

