return {
    node_name = "example_publisher",
    calibration = { 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0 },
    tags = { "sensor", "demo" },
    temperature = {
        rate = 10.0,
        base = 25.0,
        amplitude = 5.0,
        unit = "celsius"
    },
    imu = {
        rate = 2.0,
        enabled = true,
        frame_id = "imu_link"
    },
    detection = {
        rate = 1.0,
        enabled = true,
        frame_id = "camera_link"
    },
    image = {
        resolution = {
            width = 640,
            height = 480
        },
        rate = 2.0,
        flip_horizontal = false
    }
}
