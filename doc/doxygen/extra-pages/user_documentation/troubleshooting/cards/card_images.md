@page troubleshooting_card_images Card Images

# Specific printing missing

[Is your card database up-to-date?](\ref how_to_update_the_card_database)

# All Printings appear the same

If all printings for a card appear to share the same images, despite you knowing that the printings should differ, it's
possible that your card image cache got corrupted, in which case you need
to [clear your cache](\ref how_to_clear_the_image_cache). The only time this should happen is when the PictureLoader
times out trying to load a specific image or when the specific set is disabled (
see [Troubleshooting- How to enable Sets](\ref how_to_enable_sets)). both of these cases cause the
PictureLoader to fall back to the next set in the priority list, in which case an incorrect image will be saved to the
cache.

## How to clear the image cache {#how_to_clear_the_image_cache}

You can clear the image cache by navigating to "Cockatrice → Settings" in the menu bar and then clicking "Delete
Downloaded Images" under the "Card Sources" settings tab. This will delete the network cache and additionally force a
refresh of the in-memory cache (effectively forcing every picture to be loaded again).

# Card images do not load

If the cards exist in your database and their entries are correct but the images just won't load (i.e. they stay as
cardbacks), then it is time to examine how Cockatrice and thus your computer connects to the backends which supply
images to Cockatrice.

## How images are loaded

When Cockatrice receives a request to display a card image, Cockatrice first looks to the local custom picture folder,
then to the local network image cache (images it has previously downloaded). If it does not find an image locally, it
constructs an URL to download the image from a card image backend.

### Card Image Backends

These card image backends are configurable, as are the parameters that Cockatrice will supply to the card image backend
to get it to respond with the correct image (i.e. printing).

You can configure them by going to "Cockatrice → Settings" and then adjusting the list in the "Card Sources" settings
tab, with backends at the top of the list receiving higher priority.

### How URLs are constructed

To attempt to load an image, Cockatrice will take the first backend in the list. It will then take the configured
template URL and, for each set entry of the card, use the card database entry properties to substitute the template
parameters.
See [this page](https://github.com/Cockatrice/Cockatrice/wiki/Custom-Picture-Download-URLs) for more information on
template parameters. It will then order these URLs according to the user defined set preference and finally, bump the
specific URL that belongs to the set with the corresponding providerId to the top if one was supplied.
Cockatrice then attempts to load 
