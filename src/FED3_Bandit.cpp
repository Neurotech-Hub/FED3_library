#include "FED3.h"

// Initialize Bandit task settings
void FED3::initBanditTask(int pellets_per_block, bool allow_repeats)
{
    countAllPokes = false;               // Only count pokes during active trials
    pelletsToSwitch = pellets_per_block; // Number of pellets before probability switch
    allowBlockRepeat = allow_repeats;    // Whether same probabilities can repeat
    prob_left = 80;                      // Initial probability of left poke
    prob_right = 20;                     // Initial probability of right poke
    BlockPelletCount = 0;                // Reset block counter
}

// Handle block transitions and probability updates
void FED3::updateBanditBlock()
{
    static const int probs[2] = {80, 20}; // Standard probability options

    if (BlockPelletCount >= pelletsToSwitch)
    {
        BlockPelletCount = 0;               // Reset block counter
        int new_prob = probs[random(0, 2)]; // Select new probability

        // If repeats not allowed, ensure new prob is different
        if (!allowBlockRepeat)
        {
            while (new_prob == prob_left)
            {
                new_prob = probs[random(0, 2)];
            }
        }

        // Update probabilities (always sum to 100)
        prob_left = new_prob;
        prob_right = 100 - prob_left;
    }
}

// Handle Bandit-specific poke behavior
bool FED3::handleBanditPoke(bool isLeftPoke)
{
    int poke_prob = isLeftPoke ? prob_left : prob_right;
    bool success = false;

    // Log the poke
    if (isLeftPoke)
    {
        logLeftPoke();
        Event = "Left";
    }
    else
    {
        logRightPoke();
        Event = "Right";
    }

    delay(1000); // Standard delay after poke

    // Check if reward should be delivered based on probability
    if (random(100) < poke_prob)
    {
        ConditionedStimulus(); // Visual/audio feedback
        Feed();                // Deliver pellet
        BlockPelletCount++;    // Increment block counter
        success = true;
    }
    else
    {
        Tone(300, 600);          // Error tone
        Timeout(10, true, true); // 10s timeout with white noise
    }

    return success;
}