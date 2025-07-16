let vacuumOn = 0;
let autoMode = false;
let joyX = 0;
let joyY = 0;

let socket = new WebSocket('ws://' + "192.168.103.189" + ':81/');
socket.onopen = () => console.log('Connected to robot');

function sendState() {
  socket.send(JSON.stringify({
    x: joyX,
    y: joyY,
    start: vacuumOn,
    auto: autoMode
  }));
}

function toggleVacuum() {
  vacuumOn = vacuumOn ? 0 : 1;
  sendState();
}

function stopMotors() {
  joyX = 0;
  joyY = 0;
  sendState();
}

function toggleAuto() {
  autoMode = !autoMode;
  sendState();
}

const joystick = nipplejs.create({
  zone: document.getElementById('joystick-zone'),
  mode: 'static',
  position: { left: '50%', top: '50%' },
  color: 'white'
});

joystick.on('move', (evt, data) => {
  if (data && data.vector) {
    joyX = Math.floor(data.vector.x * 100);
    joyY = Math.floor(data.vector.y * -100); // invert Y if needed
    sendState();
  }
});

joystick.on('end', () => {
  joyX = 0;
  joyY = 0;
  sendState();
});