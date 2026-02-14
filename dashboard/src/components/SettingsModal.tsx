import { X } from "lucide-react";
import { useEffect, useState } from "react";
import { cn } from "../lib/utils";  // Assuming we can use cn, otherwise strict strings

type SettingsModalProps = {
    isOpen: boolean;
    onClose: () => void;
    onSave: (settings: AlertSettings) => void;
    currentSettings: AlertSettings;
};

export type AlertSettings = {
    tempMin: number;
    tempMax: number;
    humidMin: number;
    humidMax: number;
    notificationsEnabled: boolean;
};

export function SettingsModal({ isOpen, onClose, onSave, currentSettings }: SettingsModalProps) {
    const [settings, setSettings] = useState<AlertSettings>(currentSettings);

    useEffect(() => {
        setSettings(currentSettings);
    }, [currentSettings, isOpen]);

    if (!isOpen) return null;

    const handleChange = (key: keyof AlertSettings, value: number | boolean) => {
        setSettings((prev) => ({ ...prev, [key]: value }));
    };

    const handleSave = () => {
        onSave(settings);
        onClose();
    };

    return (
        <div className="fixed inset-0 z-[100] flex items-center justify-center bg-slate-900/60 backdrop-blur-md animate-in fade-in duration-200 p-4">
            <div className="w-full max-w-md bg-white rounded-3xl shadow-2xl animate-in zoom-in-95 duration-200 overflow-hidden">

                {/* Header */}
                <div className="px-6 py-4 border-b border-slate-100 flex items-center justify-between bg-slate-50/50">
                    <h2 className="text-lg font-bold text-slate-900">Configure Alerts</h2>
                    <button
                        onClick={onClose}
                        className="p-2 -mr-2 text-slate-400 hover:text-slate-600 hover:bg-slate-100 rounded-full transition-all"
                    >
                        <X size={20} />
                    </button>
                </div>

                {/* Content */}
                <div className="p-6 space-y-6">
                    {/* Temperature */}
                    <div className="space-y-3">
                        <label className="text-xs font-bold text-slate-500 uppercase tracking-wider flex items-center gap-2">
                            <span className="w-1.5 h-1.5 rounded-full bg-blue-500"></span> Temperature Thresholds (°C)
                        </label>
                        <div className="grid grid-cols-2 gap-4">
                            <div className="space-y-1.5">
                                <label className="text-xs font-semibold text-slate-500 ml-1">Min</label>
                                <input
                                    type="number"
                                    value={settings.tempMin}
                                    onChange={(e) => handleChange("tempMin", Number(e.target.value))}
                                    className="w-full bg-slate-50 border border-slate-200 hover:border-blue-400 focus:border-blue-500 rounded-xl px-4 py-2.5 text-sm font-semibold text-slate-900 font-mono-nums focus:ring-4 focus:ring-blue-500/10 focus:outline-none transition-all"
                                />
                            </div>
                            <div className="space-y-1.5">
                                <label className="text-xs font-semibold text-slate-500 ml-1">Max</label>
                                <input
                                    type="number"
                                    value={settings.tempMax}
                                    onChange={(e) => handleChange("tempMax", Number(e.target.value))}
                                    className="w-full bg-slate-50 border border-slate-200 hover:border-red-400 focus:border-red-500 rounded-xl px-4 py-2.5 text-sm font-semibold text-slate-900 font-mono-nums focus:ring-4 focus:ring-red-500/10 focus:outline-none transition-all"
                                />
                            </div>
                        </div>
                    </div>

                    {/* Humidity */}
                    <div className="space-y-3">
                        <label className="text-xs font-bold text-slate-500 uppercase tracking-wider flex items-center gap-2">
                            <span className="w-1.5 h-1.5 rounded-full bg-cyan-500"></span> Humidity Thresholds (%)
                        </label>
                        <div className="grid grid-cols-2 gap-4">
                            <div className="space-y-1.5">
                                <label className="text-xs font-semibold text-slate-500 ml-1">Min</label>
                                <input
                                    type="number"
                                    value={settings.humidMin}
                                    onChange={(e) => handleChange("humidMin", Number(e.target.value))}
                                    className="w-full bg-slate-50 border border-slate-200 hover:border-cyan-400 focus:border-cyan-500 rounded-xl px-4 py-2.5 text-sm font-semibold text-slate-900 font-mono-nums focus:ring-4 focus:ring-cyan-500/10 focus:outline-none transition-all"
                                />
                            </div>
                            <div className="space-y-1.5">
                                <label className="text-xs font-semibold text-slate-500 ml-1">Max</label>
                                <input
                                    type="number"
                                    value={settings.humidMax}
                                    onChange={(e) => handleChange("humidMax", Number(e.target.value))}
                                    className="w-full bg-slate-50 border border-slate-200 hover:border-cyan-400 focus:border-cyan-500 rounded-xl px-4 py-2.5 text-sm font-semibold text-slate-900 font-mono-nums focus:ring-4 focus:ring-cyan-500/10 focus:outline-none transition-all"
                                />
                            </div>
                        </div>
                    </div>

                    {/* Notifications Toggle */}
                    <div className="flex items-center justify-between pt-2 px-1">
                        <span className="text-sm font-semibold text-slate-700">Enable Alert Notifications</span>
                        <button
                            onClick={() => handleChange("notificationsEnabled", !settings.notificationsEnabled)}
                            className={`relative inline-flex h-7 w-12 items-center rounded-full transition-colors focus:outline-none focus:ring-4 focus:ring-emerald-500/20 ${settings.notificationsEnabled ? "bg-emerald-500" : "bg-slate-200"
                                }`}
                        >
                            <span
                                className={`inline-block h-5 w-5 transform rounded-full bg-white shadow-sm transition-transform ${settings.notificationsEnabled ? "translate-x-6" : "translate-x-1"
                                    }`}
                            />
                        </button>
                    </div>
                </div>

                {/* Footer */}
                <div className="px-6 py-4 bg-slate-50 border-t border-slate-100 flex justify-end gap-3">
                    <button
                        onClick={onClose}
                        className="px-5 py-2.5 text-sm font-bold text-slate-600 bg-white border border-slate-200 hover:border-slate-300 hover:bg-slate-50 rounded-xl transition-all shadow-sm"
                    >
                        Cancel
                    </button>
                    <button
                        onClick={handleSave}
                        className="px-6 py-2.5 text-sm font-bold text-white bg-blue-600 hover:bg-blue-700 active:scale-95 rounded-xl transition-all shadow-md shadow-blue-500/20"
                    >
                        Save Changes
                    </button>
                </div>
            </div>
        </div>
    );
}
