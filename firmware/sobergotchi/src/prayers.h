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
  {"Third Step", "God", "{word}, I offer myself to Thee, to build with me and to do with me as Thou wilt. "
                              "Relieve me of the bondage of self, that I may better do Thy will. "
                              "Take away my difficulties, that victory over them may bear witness to those I would help "
                              "of Thy Power, Thy Love, and Thy Way of life. May I do Thy will always!"},
  {"Seventh Step", "My Creator", "{word}, I am now willing that you should have all of me, good and bad. "
                                       "I pray that you now remove from me every single defect of character which "
                                       "stands in the way of my usefulness to you and my fellows. Grant me strength, "
                                       "as I go out from here, to do your bidding. Amen."},
  {"Eleventh Step", "Lord", "{word}, make me a channel of Thy peace; "
                            "that where there is hatred, I may bring love; "
                            "that where there is wrong, I may bring the spirit of forgiveness; "
                            "that where there is discord, I may bring harmony; "
                            "that where there is error, I may bring truth; "
                            "that where there is doubt, I may bring faith; "
                            "that where there is despair, I may bring hope; "
                            "that where there are shadows, I may bring light; "
                            "that where there is sadness, I may bring joy. "
                            "{word}, grant that I may seek rather "
                            "to comfort, than to be comforted; "
                            "to understand, than to be understood; "
                            "to love, than to be loved. "
                            "For it is by self-forgetting, that one finds. "
                            "It is by forgiving, that one is forgiven. "
                            "It is by dying, that one awakens to Eternal Life. "
                            "Amen."},
};
