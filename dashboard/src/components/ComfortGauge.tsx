import { RadialBarChart, RadialBar, PolarAngleAxis, ResponsiveContainer } from "recharts";
import { cn } from "../lib/utils";
import { memo } from "react";

type ComfortGaugeProps = {
    score: number;
    label: string;
    className?: string;
};

export const ComfortGauge = memo(function ComfortGauge({ score, label, className }: ComfortGaugeProps) {
    const safeScore = Math.min(Math.max(score, 0), 100);

    let color = "#10b981";
    if (safeScore < 40) color = "#ef4444";
    else if (safeScore < 70) color = "#f59e0b";

    const data = [
        { name: "Score", value: safeScore, fill: color }
    ];

    return (
        <div className={cn("relative flex items-center justify-center h-[240px] w-full", className)}>
            <div className="absolute inset-0 flex flex-col items-center justify-center pointer-events-none z-10 transition-colors duration-500">
                {/* Score number — black/heavy font */}
                <span className="text-5xl font-black tracking-tight font-mono-nums" style={{ color }}>
                    {Math.round(safeScore)}
                </span>
                {/* Subtitle — bold, tracking wide */}
                <span className="text-[10px] font-bold text-slate-400 dark:text-slate-500 uppercase tracking-[0.15em] mt-1.5 transition-colors">
                    Comfort Index
                </span>
                {/* Label badge */}
                <div className={cn(
                    "mt-3 px-4 py-1.5 rounded-full text-[10px] font-black uppercase tracking-wider transition-colors",
                    color === "#ef4444" ? "bg-red-50 dark:bg-red-900/30 text-red-600 dark:text-red-400" :
                        color === "#f59e0b" ? "bg-amber-50 dark:bg-amber-900/30 text-amber-600 dark:text-amber-400" :
                            "bg-emerald-50 dark:bg-emerald-900/30 text-emerald-600 dark:text-emerald-400"
                )}>
                    {label}
                </div>
            </div>

            <ResponsiveContainer width="100%" height="100%">
                <RadialBarChart
                    innerRadius="85%"
                    outerRadius="100%"
                    barSize={14}
                    data={data}
                    startAngle={90}
                    endAngle={-270}
                >
                    <PolarAngleAxis type="number" domain={[0, 100]} angleAxisId={0} tick={false} />

                    <RadialBar
                        background={{ fill: "var(--gauge-bg)" }}
                        dataKey="value"
                        cornerRadius={12}
                        animationDuration={800}
                        animationEasing="ease-out"
                    />
                </RadialBarChart>
            </ResponsiveContainer>
        </div>
    );
});
