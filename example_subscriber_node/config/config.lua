return {
    node_name = "example_subscriber",
    alert_targets = { "console", "log" },
    display = {
        interval = 2.0,
        show_temperature = true,
        show_imu = true,
        show_detections = true,
        show_image = true
    },
    thresholds = {
        temperature = {
            high = 30.0,
            low = 20.0
        },
        detection_min_confidence = 0.5
    }
}
