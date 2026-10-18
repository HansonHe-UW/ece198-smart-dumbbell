# Motion model

Status: no recordings, training pipeline, trained model or evaluation results committed.

Classes: curl, idle, other. Collect data with the intended mounting orientation and different movement speeds. Include pauses, partial movements, pickup and put-down as appropriate examples. Define labels before collecting data.

Split by participant or full recording session before windowing. Fit preprocessing only on training data. Evaluate held-out sessions, report confusion matrices, and measure model size and inference latency on Nano ESP32. Classification and repetition counting are separate tasks.

Do not commit identifiable recordings. See [data guidance](../data/README.md).
