// The Step prayers on Your path. Paste each prayer's words in place of the placeholder in brackets.
// Write {word} where the prayer names a higher power (for example at the start): it becomes the word picked in
// Your word, or the prayer's own word below when none has been picked (or "none for now" was picked).
// Keep each prayer under about 850 characters; the screen turns pages by itself.
#pragma once

struct Prayer {
  const char* title;
  const char* word;   // the prayer's own word, used until Your word is chosen
  const char* text;
};

static const Prayer PRAYERS[3] = {
  {"Third Step", "God", "{word}, [paste the Third Step prayer here]"},
  {"Seventh Step", "My Creator", "{word}, [paste the Seventh Step prayer here]"},
  {"Eleventh Step", "Lord", "{word}, [paste the Eleventh Step prayer here]"},
};
