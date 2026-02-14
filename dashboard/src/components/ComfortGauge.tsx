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
            <div className="absolute inset-0 flex flex-col items-center justify-center pointer-events-none z-10">
                <span className="text-5xl font-black tracking-tighter font-mono-nums" style={{ color }}>
                    {Math.round(safeScore)}
                </span>
                <span className="text-xs font-bold text-slate-400 uppercase tracking-widest mt-1">
                    Comfort Index
                </span>
                <div className={cn(
                    "mt-2 px-3 py-1 rounded-full text-[10px] font-extrabold uppercase tracking-wide",
                    color === "#ef4444" ? "bg-red-50 text-red-600" :
                        color === "#f59e0b" ? "bg-amber-50 text-amber-600" :
                            "bg-emerald-50 text-emerald-600"
                )}>
                    {label}
                </div>
            </div>

            <ResponsiveContainer width="100%" height="100%">
                <RadialBarChart
                    innerRadius="85%"
                    outerRadius="100%"
                    barSize={16}
                    data={data}
                    startAngle={90}
                    endAngle={-270}
                >
                    <PolarAngleAxis type="number" domain={[0, 100]} angleAxisId={0} tick={false} />

                    <RadialBar
                        background={{ fill: "#f1f5f9" }}
                        dataKey="value"
                        cornerRadius={10}
                        animationDuration={800}
                        animationEasing="ease-out"
                    />
                </RadialBarChart>
            </ResponsiveContainer>
        </div>
    );
});
