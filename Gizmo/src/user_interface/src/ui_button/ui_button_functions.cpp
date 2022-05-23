#include "Arduino.h"
#include "ui_button_class.h"

// Button Interrupt Service Routine
void buttonISR(void)
{
	ui_button_class& button = ui_button_class::getButton();
	button.registerPress();
}

ui_button_class::ui_button_class()
{
	ui_button_setup();
}

static ui_button_class& ui_button_class::getButton()
{
  static ui_button_class button;
  return button;
}

void ui_button_class::registerPress()
{
  unsigned long currentMillis = millis();
  if(waitingForRelease)
  {
    if(clickTimes[1] == 0 && clickTimes[0] == 0 && currentMillis - clickTimes[1] > BUTTON_DEBOUNCE) //catch second click release
    {
      releaseTime = currentMillis;
      waitingForRelease = false;
      return;
    }
    else if(clickTimes[1] == 0 && clickTimes[0] != 0 && (currentMillis - clickTimes[0] > BUTTON_DEBOUNCE)) //catch first click release
    {
      releaseTime = currentMillis;
      waitingForRelease = false;
      return;
    }
    else
      return;
  }

  if(currentMillis - releaseTime < BUTTON_DEBOUNCE)
    return;

  if(clickTimes[0] == 0)
  {
    clickTimes[0] = currentMillis;
    waitingForRelease = true;
  }
  else if(clickTimes[1] == 0 && clickTimes[0] != 0 && currentMillis - clickTimes[0] > BUTTON_DEBOUNCE)
  {
    clickTimes[1] = currentMillis;
    waitingForRelease = true;
  }
    setButtonStatus();
}

bool ui_button_class::checkSelect()
{
    bool temp = select;
    if (select) 
	{
      next = false;
      select = false;
    }
    return temp;
}

bool ui_button_class::checkNext()
{
    bool temp = next;
    if (next) 
	{
      next = false;
      select = false;
    }
    return temp;
}

void ui_button_class::setButtonStatus()
{
  interrupts();
  while(!next && !select)
  {
    if((millis() - clickTimes[0]) > SELECT_DELTA)
    {
      //1 click => NEXT
      next = true;
      clickTimes[0] = 0;
      clickTimes[1] = 0;
    }
    else if(clickTimes[1] > 0 && (clickTimes[1] - clickTimes[0]) < SELECT_DELTA)
    {
      //2 clicks => SELECT
      select = true;
      clickTimes[0] = 0;
      clickTimes[1] = 0;
    }
  }
}

void ui_button_class::ui_button_setup()
{
  pinMode(BUTTON_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, HIGH);
}
