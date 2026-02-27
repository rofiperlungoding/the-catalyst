export type Device = {
    id: string;
    device_name: string;
    mac_address: string;
    status: 'online' | 'offline' | 'maintenance' | 'error' | 'provisioning';
    ip_address: string | null;
    last_seen_at: string;
    created_at: string;
};

export type SensorReading = {
    id: number;
    device_id: string;
    temperature: number;
    humidity: number;
    heat_index: number;
    dew_point: number;
    comfort_score: number;
    comfort_lvl: 'freezing' | 'cold' | 'cool' | 'comfortable' | 'warm' | 'hot' | 'extreme';
    recorded_at: string;
};

export type DeviceHealth = {
    id: number;
    device_id: string;
    free_heap_bytes: number;
    uptime_ms: number;
    wifi_rssi: number;
    recorded_at: string;
};
export type BroneTask = {
    id: string;
    course_name: string;
    task_title: string;
    deadline: string;
    status: 'pending' | 'completed';
    updated_at: string;
};

export type ClassSchedule = {
    id: string;
    day_of_week: string;
    subject: string;
    start_time: string;
    end_time: string;
    room: string;
    created_at: string;
};
