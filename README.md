These projects all target the Arduino Nano for controlling various aspects of my model railways.

UpperRailwaySignalling uses the Arduino to control a 3 aspect signal along with ground signals, using occupancy sensors and turnout positions to set the signal aspects accordingly.
TrainDetection uses an external multiplexer board to detect the positions of trains in hidden storage. Multiple laser detectors feed into the board, with occupancy being shown on an LCD display.
LowerRailwaySignalling takes DCC signals to control signals on the layout; each signal aspect is linked to a DCC address with the route setting in JMRI setting the appropriate aspects.
PointsController also takes DCC signals, changing turnouts using servos in response to DCC accessory messages.
