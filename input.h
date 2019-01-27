// Key pinout
#define K_LEFT      6
#define K_RIGHT     7
#define K_UP        8
#define K_DOWN      9
#define K_FIRE      10

void setupInput(); 
void readInput();

// Input states
boolean p_up = false;
boolean p_down = false;
boolean p_left = false;
boolean p_right = false;
boolean p_fire = false;

void setupInput() {
  // Enable pin inputs
  pinMode(K_LEFT, INPUT);
  pinMode(K_RIGHT, INPUT);
  pinMode(K_UP, INPUT);
  pinMode(K_DOWN, INPUT);
  pinMode(K_FIRE, INPUT);
}

void readInput() {
  p_left = digitalRead(K_LEFT) == HIGH;
  p_right = digitalRead(K_RIGHT) == HIGH;
  p_up = digitalRead(K_UP) == HIGH;
  p_down = digitalRead(K_DOWN) == HIGH;
  p_fire = digitalRead(K_FIRE) == HIGH;

  // For debugging
  /*
  Serial.print(p_left);
  Serial.print(" - ");
  Serial.print(p_right);
  Serial.print(" - ");
  Serial.print(p_up);
  Serial.print(" - ");
  Serial.print(p_down);
  Serial.print(" - ");
  Serial.print(p_fire);
  Serial.println();
  */
}
