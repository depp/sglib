# Audio mixer

An audio mixer translates a stream of timestamped mixing commands into a stream of audio data.

## Channels and parameters

A mixer has a fixed number of channels.
Each channel can play a single source of audio at a time.
Commands can start or stop audio on a channel, or adjust channel parameters such as volume and pan.

## Timing

There are two common use cases: *live* and *offline*.
Both use cases are supported.

A *live mixer* produces audio in real time.
To do this, commands are delayed so that they arrive in time to be rendered; delaying the output is unacceptable.
If the mixer delay is too small, the relative ordering of timestamps will change, but it is just as bad to make the delay too large.
Live mixers also account for clock drift and jitter, if the system issuing commands uses a different clock from the audio system.

An *offline mixer* waits to produce a chunk of audio until all the necessary commands have been received.

The *commit time* for a mixer is a bound for the timestamps on new commands.
Commands may only be issued with timestamps after the commit time, and mixers will try to only render audio from before the commit time.
This way, new commands will not affect already rendered audio.
Live mixers adjust the delay to keep rendered audio before the commit time, and offline mixers will wait to render audio until the commit time advances.

## Components

* `msg.h` provides the mixer commands, also called "messages".

* `queue.h` provides a thread-safe queue for messages.

* `time.h` automatically adjusts the mixer delay in live mixers.

* `audio.h` renders commands to an audio stream.
