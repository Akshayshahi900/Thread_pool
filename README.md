# Simple Thread Pool

Spawns N worker threads. Each worker waits for jobs from a channel.
When you call execute(closure), it sends the closure to the channel.
A free worker picks it up and runs it.

Why it's useful: Run async tasks without spawning new threads every time.

Performance: Basic mpsc channel, all workers share one receiver.