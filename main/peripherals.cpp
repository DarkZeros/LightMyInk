#include "peripherals.h"
#include "hardware.h"

#include "driver/gpio.h"
#include "Arduino.h"

void Peripherals::vibrator(std::vector<int> pattern) {
  constexpr const gpio_config_t kConf = {
    .pin_bit_mask = (1ULL<<HW::kVibratorPin),
    .mode = GPIO_MODE_OUTPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE,
  };

  //Configure GPIO with the given settings
  gpio_config(&kConf);

  for(auto i = 0; i < pattern.size(); i++) {
    gpio_set_level((gpio_num_t)HW::kVibratorPin, i & 1 ? 0 : 1);
    //delay(pattern[i]);
    esp_sleep_enable_timer_wakeup(pattern[i] * 1000);
    esp_light_sleep_start();
  }

  gpio_set_level((gpio_num_t)HW::kVibratorPin, 0);
}

void Peripherals::speaker(std::vector<std::pair<int, int>> pattern) {
  // constexpr const gpio_config_t kConf = {
  //   .pin_bit_mask = (1ULL<<HW::kSpeakerPin),
  //   .mode = GPIO_MODE_OUTPUT,
  //   .pull_up_en = GPIO_PULLUP_DISABLE,
  //   .pull_down_en = GPIO_PULLDOWN_DISABLE,
  //   .intr_type = GPIO_INTR_DISABLE,
  // };
  for(auto& [note, duration] : pattern) {
    if (note == 0) {
      esp_sleep_enable_timer_wakeup(duration * 1000);
      esp_light_sleep_start();
    } else {
      tone(HW::kSpeakerPin, note, duration);
      // TODO Sleep properly
      delay(duration);
      // esp_sleep_enable_timer_wakeup(duration * 1000);
      // esp_light_sleep_start();
    }
  }
  noTone(HW::kSpeakerPin);
}


#define NOTE_B0  31
#define NOTE_C1  33
#define NOTE_CS1 35
#define NOTE_D1  37
#define NOTE_DS1 39
#define NOTE_E1  41
#define NOTE_F1  44
#define NOTE_FS1 46
#define NOTE_G1  49
#define NOTE_GS1 52
#define NOTE_A1  55
#define NOTE_AS1 58
#define NOTE_B1  62
#define NOTE_C2  65
#define NOTE_CS2 69
#define NOTE_D2  73
#define NOTE_DS2 78
#define NOTE_E2  82
#define NOTE_F2  87
#define NOTE_FS2 93
#define NOTE_G2  98
#define NOTE_GS2 104
#define NOTE_A2  110
#define NOTE_AS2 117
#define NOTE_B2  123
#define NOTE_C3  131
#define NOTE_CS3 139
#define NOTE_D3  147
#define NOTE_DS3 156
#define NOTE_E3  165
#define NOTE_F3  175
#define NOTE_FS3 185
#define NOTE_G3  196
#define NOTE_GS3 208
#define NOTE_A3  220
#define NOTE_AS3 233
#define NOTE_B3  247
#define NOTE_C4  262
#define NOTE_CS4 277
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define NOTE_DS5 622
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_A5  880
#define NOTE_AS5 932
#define NOTE_B5  988
#define NOTE_C6  1047
#define NOTE_CS6 1109
#define NOTE_D6  1175
#define NOTE_DS6 1245
#define NOTE_E6  1319
#define NOTE_F6  1397
#define NOTE_FS6 1480
#define NOTE_G6  1568
#define NOTE_GS6 1661
#define NOTE_A6  1760
#define NOTE_AS6 1865
#define NOTE_B6  1976
#define NOTE_C7  2093
#define NOTE_CS7 2217
#define NOTE_D7  2349
#define NOTE_DS7 2489
#define NOTE_E7  2637
#define NOTE_F7  2794
#define NOTE_FS7 2960
#define NOTE_G7  3136
#define NOTE_GS7 3322
#define NOTE_A7  3520
#define NOTE_AS7 3729
#define NOTE_B7  3951
#define NOTE_C8  4186
#define NOTE_CS8 4435
#define NOTE_D8  4699
#define NOTE_DS8 4978
#define REST 0

// change this to make the song slower or faster
int tempo=144; 

// change this to whichever pin you want to use
int buzzer = 11;

// notes of the moledy followed by the duration.
// a 4 means a quarter note, 8 an eighteenth , 16 sixteenth, so on
// !!negative numbers are used to represent dotted notes,
// so -4 means a dotted quarter note, that is, a quarter plus an eighteenth!!
int melody[] = {

  //Based on the arrangement at https://www.flutetunes.com/tunes.php?id=192
  
  NOTE_E5, 4,  NOTE_B4,8,  NOTE_C5,8,  NOTE_D5,4,  NOTE_C5,8,  NOTE_B4,8,
  NOTE_A4, 4,  NOTE_A4,8,  NOTE_C5,8,  NOTE_E5,4,  NOTE_D5,8,  NOTE_C5,8,
  NOTE_B4, -4,  NOTE_C5,8,  NOTE_D5,4,  NOTE_E5,4,
  NOTE_C5, 4,  NOTE_A4,4,  NOTE_A4,8,  NOTE_A4,4,  NOTE_B4,8,  NOTE_C5,8,

  NOTE_D5, -4,  NOTE_F5,8,  NOTE_A5,4,  NOTE_G5,8,  NOTE_F5,8,
  NOTE_E5, -4,  NOTE_C5,8,  NOTE_E5,4,  NOTE_D5,8,  NOTE_C5,8,
  NOTE_B4, 4,  NOTE_B4,8,  NOTE_C5,8,  NOTE_D5,4,  NOTE_E5,4,
  NOTE_C5, 4,  NOTE_A4,4,  NOTE_A4,4, REST, 4,

  NOTE_E5, 4,  NOTE_B4,8,  NOTE_C5,8,  NOTE_D5,4,  NOTE_C5,8,  NOTE_B4,8,
  NOTE_A4, 4,  NOTE_A4,8,  NOTE_C5,8,  NOTE_E5,4,  NOTE_D5,8,  NOTE_C5,8,
  NOTE_B4, -4,  NOTE_C5,8,  NOTE_D5,4,  NOTE_E5,4,
  NOTE_C5, 4,  NOTE_A4,4,  NOTE_A4,8,  NOTE_A4,4,  NOTE_B4,8,  NOTE_C5,8,

  NOTE_D5, -4,  NOTE_F5,8,  NOTE_A5,4,  NOTE_G5,8,  NOTE_F5,8,
  NOTE_E5, -4,  NOTE_C5,8,  NOTE_E5,4,  NOTE_D5,8,  NOTE_C5,8,
  NOTE_B4, 4,  NOTE_B4,8,  NOTE_C5,8,  NOTE_D5,4,  NOTE_E5,4,
  NOTE_C5, 4,  NOTE_A4,4,  NOTE_A4,4, REST, 4,
  

  NOTE_E5,2,  NOTE_C5,2,
  NOTE_D5,2,   NOTE_B4,2,
  NOTE_C5,2,   NOTE_A4,2,
  NOTE_GS4,2,  NOTE_B4,4,  REST,8, 
  NOTE_E5,2,   NOTE_C5,2,
  NOTE_D5,2,   NOTE_B4,2,
  NOTE_C5,4,   NOTE_E5,4,  NOTE_A5,2,
  NOTE_GS5,2,

};

#include "driver/ledc.h"

#define LEDC_LS_TIMER LEDC_TIMER_0
#define LEDC_LS_MODE LEDC_LOW_SPEED_MODE
#define LEDC_LS_CH2_GPIO (26)
#define LEDC_LS_CH2_CHANNEL LEDC_CHANNEL_2

ledc_channel_config_t ledc_channel[1] = {
    {
        .gpio_num = LEDC_LS_CH2_GPIO,
        .speed_mode = LEDC_LS_MODE,
        .channel = LEDC_LS_CH2_CHANNEL,
        .timer_sel = LEDC_LS_TIMER,
        .duty = 0,
        .hpoint = 0,
        .flags = 0,
    },
};

ledc_timer_config_t ledc_timer = {
    .speed_mode = LEDC_LS_MODE,          // timer mode
    .duty_resolution = LEDC_TIMER_2_BIT, // resolution of PWM duty
    .timer_num = LEDC_LS_TIMER,          // timer index
    .freq_hz = 100,                      // frequency of PWM signal
    .clk_cfg = LEDC_USE_RC_FAST_CLK,     // Force source clock to RTC8M
};

void start_speaker(uint32_t freq)
{
  ledc_timer.freq_hz = freq;
  ledc_timer_config(&ledc_timer);
  ledc_channel_config(&ledc_channel[0]);
  ledc_set_duty(ledc_channel[0].speed_mode, ledc_channel[0].channel, 1);
  ledc_update_duty(ledc_channel[0].speed_mode, ledc_channel[0].channel);
}

void stop_speaker()
{
  ledc_set_duty(ledc_channel[0].speed_mode, ledc_channel[0].channel, 0);
  ledc_update_duty(ledc_channel[0].speed_mode, ledc_channel[0].channel);
}

void Peripherals::tetris() {
  int notes=sizeof(melody)/sizeof(melody[0])/2; 

  //ESP_ERROR_CHECK(esp_sleep_pd_config(ESP_PD_DOMAIN_RTC8M, ESP_PD_OPTION_ON));
  //ESP_ERROR_CHECK(gpio_sleep_sel_dis((gpio_num_t)HW::kSpeakerPin)); // Needed for light sleep.

  // this calculates the duration of a whole note in ms (60s/tempo)*4 beats
  int wholenote = (60000 * 4) / tempo;

  int divider = 0, noteDuration = 0;
  for (int thisNote = 0; thisNote < notes * 2; thisNote = thisNote + 2) {

    // calculates the duration of each note
    divider = melody[thisNote + 1];
    if (divider > 0) {
      // regular note, just proceed
      noteDuration = (wholenote) / divider;
    } else if (divider < 0) {
      // dotted notes are represented with negative durations!!
      noteDuration = (wholenote) / abs(divider);
      noteDuration *= 1.5; // increases the duration in half for dotted notes
    }

    // we only play the note for 90% of the duration, leaving 10% as a pause
    tone(HW::kSpeakerPin, melody[thisNote], noteDuration*0.9);
    //ledcAttach(HW::kSpeakerPin, melody[thisNote], 10);
    //ledcWriteTone(HW::kSpeakerPin, melody[thisNote]);
    //delay(noteDuration);
    //start_speaker(melody[thisNote]);
    //esp_sleep_enable_timer_wakeup(noteDuration*0.9*1000); //light sleep for 2 seconds
    //esp_light_sleep_start();

    // Wait for the specific duration before playing the next note.
    delay(noteDuration);

    //stop_speaker();
    //ledcDetach(HW::kSpeakerPin);
    
    // stop the waveform generation before the next note.
    // noTone(HW::kSpeakerPin);
  }
}

void Peripherals::light() {

}