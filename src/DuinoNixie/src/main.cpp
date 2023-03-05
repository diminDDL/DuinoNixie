#include <DS3231.h>
#include <Wire.h>
#include <EEPROM.h>
unsigned long tim = 0;
unsigned long blink_timer = 0;
unsigned long lp_millis = 0;
byte last_minute = 0;
byte digit[4];
bool blink_flag = 1;
bool on_off_flag = 0;
bool lp_rst = 0;
bool night_mode = 0;
bool show_date = 0;
bool force_update = true;
byte night_mode_val = 140;
byte day_mode_val = 240;
byte date_tick = 0;
byte show_date_evry_sec = 20;
byte show_date_for = 5;
byte hour_offset = 0;
///////////////////////
unsigned int lp_interval = 2000;
unsigned int click_time = 300;
///////////////////////
unsigned long click_interval = 0;
bool mode = 0;
bool is_click = true;
byte type_of_scroll = 1; // 0 - no scroll // 1 - scroll all // 2 - scroll update
byte hlat_time_global = 60;

byte btn_plus = 3;
byte btn_ok = 5;
byte btn_minus = 2;

byte out1 = A3;
byte out2 = A1;
byte out4 = A0;
byte out8 = A2;

byte key1 = 8;
byte key2 = 7;
byte key3 = 6;
byte key4 = 4;
byte Hog1 = 11;
DS3231 Clock;
bool h12 = false;
bool time_mode = false;
bool PM = false;
byte month, date, hour, minute;

void setPwmFrequency(int pin, int divisor)
{
	byte mmode;
	if (pin == 5 || pin == 6 || pin == 9 || pin == 10)
	{
		switch (divisor)
		{
		case 1:
			mmode = 0x01;
			break;
		case 8:
			mmode = 0x02;
			break;
		case 64:
			mmode = 0x03;
			break;
		case 256:
			mmode = 0x04;
			break;
		case 1024:
			mmode = 0x05;
			break;
		default:
			return;
		}
		if (pin == 5 || pin == 6)
		{
			TCCR0B = (TCCR0B & 0b11111000) | mmode;
		}
		else
		{
			TCCR1B = (TCCR1B & 0b11111000) | mmode;
		}
	}
	else if (pin == 3 || pin == 11)
	{
		switch (divisor)
		{
		case 1:
			mmode = 0x01;
			break;
		case 8:
			mmode = 0x02;
			break;
		case 32:
			mmode = 0x03;
			break;
		case 64:
			mmode = 0x04;
			break;
		case 128:
			mmode = 0x05;
			break;
		case 256:
			mmode = 0x06;
			break;
		case 1024:
			mmode = 0x07;
			break;
		default:
			return;
		}
		TCCR2B = (TCCR2B & 0b11111000) | mmode;
	}
}

void setNumber(byte);
bool long_press(byte, unsigned int);
void menu();
void show(int, int);

void ReadDS3231(bool with_offset = true)
{
	minute = Clock.getMinute();
	hour = Clock.getHour(h12, PM);
	if(with_offset){
		hour = hour + hour_offset;
		if (hour > 23)
		{
			hour = hour - 24;
		}
	}
}

void setup()
{
	// TCCR1B=TCCR1B&0b11111000|0x01;
	setPwmFrequency(9, 1);
	analogWrite(9, day_mode_val);

	// Start the I2C interface
	Wire.begin();
	pinMode(out1, OUTPUT);
	pinMode(out2, OUTPUT);
	pinMode(out4, OUTPUT);
	pinMode(out8, OUTPUT);

	pinMode(key1, OUTPUT);
	pinMode(key2, OUTPUT);
	pinMode(key3, OUTPUT);
	pinMode(key4, OUTPUT);
	pinMode(Hog1, OUTPUT);
	if(EEPROM.read(0) == 255){
		// the EEPROM is empty, set the default value
		EEPROM.write(time_mode, 0);
		EEPROM.write(night_mode, 0);
		EEPROM.write(type_of_scroll, 1);
		EEPROM.write(show_date, 0);
		EEPROM.write(hour_offset, 0);
	}
	time_mode = EEPROM.read(0);
	night_mode = EEPROM.read(1);
	type_of_scroll = EEPROM.read(2);
	show_date = EEPROM.read(3);
	hour_offset = EEPROM.read(4);
	
	ReadDS3231();
	Serial.begin(115200);
	Serial.setTimeout(1);
}

void blinker()
{
	// if we have data in the serial port, read it
	if (Serial.available() > 0)
	{
		// we are expecting a string containing time in the following format: hh:mm:ss
		// read the string
		String timeStr = Serial.readString();
		// if string doesn't end with a newline we skip it
		if (timeStr[timeStr.length() - 1] == '\n'){
			// parse the string to extract the hours, minutes and seconds
			byte buff_hour = timeStr.substring(0, 2).toInt();
			byte buff_minute = timeStr.substring(3, 5).toInt();
			byte buff_second = timeStr.substring(6, 8).toInt();
			// set the time
			Clock.setHour(buff_hour);
			Clock.setMinute(buff_minute);
			Clock.setSecond(buff_second);
		}
	}
	if (millis() - blink_timer >= 1000 || force_update)
	{
		blink_timer = millis();
		blink_flag = !blink_flag;
		if (!mode)
			ReadDS3231();
	}
	if ((night_mode && hour >= 22) || (hour <= 7 && !time_mode))
	{
		analogWrite(9, night_mode_val);
	}
	else
	{
		analogWrite(9, day_mode_val);
	}
	if (time_mode && !h12)
	{
		if (hour > 12)
		{
			hour = hour - 12;
		}
	}
}

void antiotrav(int dig, int key, byte hlat_time = 60);

void quick_antiotrav(unsigned int time_mks_burn = 150)
{
	analogWrite(9, night_mode_val);
	digitalWrite(key1, HIGH);
	digitalWrite(key2, HIGH);
	digitalWrite(key3, HIGH);
	digitalWrite(key4, HIGH);
	for (byte number = 0; number < 9; number++)
	{
		setNumber(number);
		delayMicroseconds(time_mks_burn);
	}
	analogWrite(9, day_mode_val);
}

void loop()
{
	if (long_press(btn_ok, lp_interval) && digitalRead(btn_minus) == LOW && digitalRead(btn_plus) == LOW)
	{
		menu();
	}
	// second blinker and clock update
	blinker();
	if (date_tick >= show_date_evry_sec && !force_update)
	{
		if (date_tick >= show_date_evry_sec + show_date_for)
		{
			date_tick = 0;
		}
	}
	else
	{
		if (minute != last_minute || force_update)
		{
			if (type_of_scroll == 1)
			{
				antiotrav(0, key1, hlat_time_global);
				digit[0] = hour / 10;
				antiotrav(1, key2, hlat_time_global);
				digit[1] = hour % 10;
				antiotrav(2, key3, hlat_time_global);
				digit[2] = minute / 10;
				antiotrav(3, key4, hlat_time_global);
				digit[3] = minute % 10;
			}
			else if (type_of_scroll == 0)
			{
				digit[0] = hour / 10;
				digit[1] = hour % 10;
				digit[2] = minute / 10;
				digit[3] = minute % 10;
				quick_antiotrav();
			}
			else if (type_of_scroll == 2)
			{
				if (digit[0] != hour / 10)
				{
					antiotrav(0, key1, hlat_time_global);
					digit[0] = hour / 10;
				}
				else
				{
					digit[0] = hour / 10;
					quick_antiotrav();
				}
				if (digit[1] != hour % 10)
				{
					antiotrav(1, key2, hlat_time_global);
					digit[1] = hour % 10;
				}
				else
				{
					digit[1] = hour % 10;
					quick_antiotrav();
				}
				if (digit[2] != minute / 10)
				{
					antiotrav(2, key3, hlat_time_global);
					digit[2] = minute / 10;
				}
				else
				{
					digit[2] = minute / 10;
				}
				if (digit[3] != minute % 10)
				{
					antiotrav(3, key4, hlat_time_global);
					digit[3] = minute % 10;
				}
				else
				{
					digit[3] = minute % 10;
				}
			}
			force_update = false;
		}
	}
	last_minute = minute;
	show(digit[0], key1);
	show(digit[1], key2);
	show(10, Hog1);
	show(digit[2], key3);
	show(digit[3], key4);
}

void show(int number, int digit)
{
	if (number == 10)
	{
		delayMicroseconds(300);
		digitalWrite(digit, blink_flag);
		delayMicroseconds(10);
		digitalWrite(digit, LOW);
	}
	else if (number < 10)
	{
		delayMicroseconds(300);
		setNumber(number);
		digitalWrite(digit, HIGH);
		delayMicroseconds(2500);
		digitalWrite(digit, LOW);
	}
}
void setNumber(byte num)
{
	switch (num)
	{
	case 0:
		digitalWrite(out1, LOW);
		digitalWrite(out2, LOW);
		digitalWrite(out4, LOW);
		digitalWrite(out8, LOW);
		break;
	case 1:
		digitalWrite(out1, HIGH);
		digitalWrite(out2, LOW);
		digitalWrite(out4, LOW);
		digitalWrite(out8, LOW);
		break;
	case 2:
		digitalWrite(out1, LOW);
		digitalWrite(out2, HIGH);
		digitalWrite(out4, LOW);
		digitalWrite(out8, LOW);
		break;
	case 3:
		digitalWrite(out1, HIGH);
		digitalWrite(out2, HIGH);
		digitalWrite(out4, LOW);
		digitalWrite(out8, LOW);
		break;
	case 4:
		digitalWrite(out1, LOW);
		digitalWrite(out2, LOW);
		digitalWrite(out4, HIGH);
		digitalWrite(out8, LOW);
		break;
	case 5:
		digitalWrite(out1, HIGH);
		digitalWrite(out2, LOW);
		digitalWrite(out4, HIGH);
		digitalWrite(out8, LOW);
		break;
	case 6:
		digitalWrite(out1, LOW);
		digitalWrite(out2, HIGH);
		digitalWrite(out4, HIGH);
		digitalWrite(out8, LOW);
		break;
	case 7:
		digitalWrite(out1, HIGH);
		digitalWrite(out2, HIGH);
		digitalWrite(out4, HIGH);
		digitalWrite(out8, LOW);
		break;
	case 8:
		digitalWrite(out1, LOW);
		digitalWrite(out2, LOW);
		digitalWrite(out4, LOW);
		digitalWrite(out8, HIGH);
		break;
	case 9:
		digitalWrite(out1, HIGH);
		digitalWrite(out2, LOW);
		digitalWrite(out4, LOW);
		digitalWrite(out8, HIGH);
		break;
	}
}
void light_up_rest(int dig_orig)
{
	switch (dig_orig)
	{
	case 0:
		blinker();
		show(10, Hog1);
		show(digit[1], key2);
		show(digit[2], key3);
		show(digit[3], key4);
		break;
	case 1:
		blinker();
		show(10, Hog1);
		show(digit[0], key1);
		show(digit[2], key3);
		show(digit[3], key4);
		break;
	case 2:
		blinker();
		show(10, Hog1);
		show(digit[0], key1);
		show(digit[1], key2);
		show(digit[3], key4);
		break;
	case 3:
		blinker();
		show(10, Hog1);
		show(digit[0], key1);
		show(digit[1], key2);
		show(digit[2], key3);
	}
}
int sub(int to_sub)
{
	if (to_sub >= 10)
	{
		return to_sub - 10;
	}
	return to_sub;
}
void antiotrav(int dig, int key, byte hlat_time = 60)
{
	tim = millis();
	byte i = 1;
	i = digit[dig];
	for (int i = 0; i < 11; i++)
	{
		while (millis() - tim <= hlat_time)
		{
			show(sub(digit[dig] + i), key);
			light_up_rest(dig);
		}
		tim = millis();
	}
}
bool long_press(byte pin, unsigned int time_to_press)
{
	if (digitalRead(pin) == LOW)
		lp_rst = false;
	if (digitalRead(pin) == HIGH && !lp_rst)
	{
		if (!on_off_flag)
			lp_millis = millis();
		on_off_flag = true;
	}
	else
	{
		lp_millis = millis();
		on_off_flag = false;
	}
	if (on_off_flag && digitalRead(pin) == HIGH && millis() - lp_millis >= time_to_press)
	{
		lp_millis = millis();
		on_off_flag = false;
		lp_rst = true;
		is_click = false;
		return true;
	}
	else
	{
		return false;
	}
}
bool click(byte pin)
{
	if (digitalRead(pin) == HIGH && millis() - click_interval >= click_time && is_click)
	{
		click_interval = millis();
		return true;
	}

	return false;
}
void menu()
{
	mode = true;
	byte curr_menu = 0;
	bool in_menu = true;
	bool in_sub_sub_menu = true;
	while (in_menu)
	{
		switch (curr_menu)
		{
		case 0:
			// hour
			ReadDS3231(false);
			while (in_sub_sub_menu)
			{
				if (digitalRead(btn_ok) == LOW)
				{
					is_click = true;
				}
				digit[0] = hour / 10;
				digit[1] = hour % 10;
				blink_flag = true;
				if (hour < 10)
					digit[0] = 0;
				show(digit[0], key1);
				show(digit[1], key2);
				show(10, Hog1);
				if (click(btn_plus))
				{
					if (hour < 23)
						hour++;
					else
						hour = 0;
				}
				if (click(btn_minus))
				{
					if (hour > 0)
						hour--;
					else
						hour = 23;
				}
				if (click(btn_ok))
				{
					in_sub_sub_menu = false;
					Clock.setHour(hour);
				}
			}
			in_sub_sub_menu = true;
			curr_menu = 1;
			break;
		case 1:
			// minute
			while (in_sub_sub_menu)
			{
				if (digitalRead(btn_ok) == LOW)
				{
					is_click = true;
				}
				digit[2] = minute / 10;
				digit[3] = minute % 10;
				blink_flag = true;
				show(digit[2], key3);
				show(digit[3], key4);
				show(10, Hog1);
				if (click(btn_plus))
				{
					if (minute < 59)
						minute++;
					else
						minute = 0;
				}
				if (click(btn_minus))
				{
					if (minute > 0)
						minute--;
					else
						minute = 59;
				}
				if (click(btn_ok))
				{
					in_sub_sub_menu = false;
					Clock.setMinute(minute);
					Clock.setSecond(0);
				}
			}
			in_sub_sub_menu = true;
			curr_menu = 2;
			break;
		case 2:
			// 24h vs 12h
			while (in_sub_sub_menu)
			{
				if (time_mode)
				{
					show(1, key2);
					show(2, key3);
				}
				else
				{
					show(2, key2);
					show(4, key3);
				}
				if (click(btn_minus) || click(btn_plus))
				{
					time_mode = !time_mode;
				}
				if (click(btn_ok))
				{
					in_sub_sub_menu = false;
					EEPROM.write(0, time_mode);
					curr_menu = 3;
				}
			}
			break;
		case 3:
			// date
			//   while(in_sub_sub_menu){
			//   if(digitalRead(btn_ok) == LOW){
			//     is_click = true;
			//   }
			//   show(0, key1);
			//   show(0, key2);
			//   show(0, key3);
			//   show(show_date, key4);
			//   show(10, Hog1);
			//   if(click(btn_minus) || click(btn_plus))
			//     show_date = !show_date;
			//   if(click(btn_ok)){
			//     in_sub_sub_menu = false;
			//     EEPROM.write(3, show_date);
			//   }
			//   }
			// in_sub_sub_menu = true;
			// if(show_date){
			//   while(in_sub_sub_menu){
			//     show(0, key1);
			//     show(0, key2);
			//     show(0, key3);
			//     show(show_date, key4);
			//     show(10, Hog1);
			//   }
			// }
			curr_menu = 4;

			break;
		case 4:
			in_sub_sub_menu = true;
			// night mode? 1/0
			while (in_sub_sub_menu)
			{
				show(0, key1);
				show(0, key2);
				show(0, key3);
				show(night_mode, key4);
				if (click(btn_minus) || click(btn_plus))
					night_mode = !night_mode;
				if (night_mode)
				{
					analogWrite(9, night_mode_val);
				}
				else
				{
					analogWrite(9, day_mode_val);
				}
				if (click(btn_ok))
				{
					in_sub_sub_menu = false;
					EEPROM.write(1, night_mode);
					curr_menu = 5;
					analogWrite(9, day_mode_val);
				}
			}
			break;
		case 5:
			in_sub_sub_menu = true;
			// 0 - no scroll // 1 - scroll all // 2 - scroll update
			while (in_sub_sub_menu)
			{
				show(0, key1);
				show(0, key2);
				// show(0, key3);
				show(type_of_scroll, key4);
				if (click(btn_plus))
				{
					if (type_of_scroll < 2)
						type_of_scroll++;
					else
						type_of_scroll = 0;
				}
				if (click(btn_minus))
				{
					if (type_of_scroll > 0)
						type_of_scroll--;
					else
						type_of_scroll = 2;
				}
				if (click(btn_ok))
				{
					in_sub_sub_menu = false;
					EEPROM.write(2, type_of_scroll);
					curr_menu = 6;
				}
			}
			break;
		case 6:
			in_sub_sub_menu = true;
			while (in_sub_sub_menu)
			{
				show(0, key1);
				//show(0, key2);
				digit[2] = hour_offset / 10;
				digit[3] = hour_offset % 10;
				blink_flag = false;
				show(digit[2], key3);
				show(digit[3], key4);
				show(10, Hog1);

				if (click(btn_plus))
				{
					if (hour_offset < 23)
						hour_offset++;
					else
						hour_offset = 0;
				}
				if (click(btn_minus))
				{
					if (hour_offset > 0)
						hour_offset--;
					else
						hour_offset = 23;
				}

				if (click(btn_ok))
				{
					in_sub_sub_menu = false;
					EEPROM.write(4, hour_offset);
					curr_menu = 0;
					in_menu = false;
				}
			}
			break;
		}
	}
	mode = false;
	force_update = true;
}
