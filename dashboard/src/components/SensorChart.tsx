import { Area, AreaChart, ResponsiveContainer, Tooltip, XAxis, YAxis } from "recharts";
import type { SensorReading } from "../types";
import { format } from "date-fns";
import { cn } from "../lib/utils";
import { memo, useMemo } from "react";

type SensorChartProps = {
    data: SensorReading[];
    dataKey: "temperature" | "humidity" | "comfort_score";
    color?: string;
    unit?: string;
    title?: string;
    className?: string;
    height?: number | string;
};

export const SensorChart = memo(function SensorChart({ data, dataKey, color = "#3b82f6", unit, title, className, height = "100%" }: SensorChartProps) {
    // Memoize + limit to last 30 points for performance
    const chartData = useMemo(() => {
        const sliced = data.slice(0, 30);
        return sliced.reverse();
    }, [data]);

    return (
        <div className={cn("w-full h-full", className)} style={{ height }}>
            {title && (
                <div className="mb-2 flex items-center justify-between">
                    <h3 className="text-base font-semibold text-slate-900 dark:text-slate-100 transition-colors">{title}</h3>
                </div>
            )}
            <ResponsiveContainer width="100%" height="100%">
                <AreaChart data={chartData} margin={{ top: 10, right: 0, left: -25, bottom: 0 }}>
                    <defs>
                        <linearGradient id={`gradient-apple-${dataKey}`} x1="0" y1="0" x2="0" y2="1">
                            <stop offset="0%" stopColor={color} stopOpacity={0.25} />
                            <stop offset="100%" stopColor={color} stopOpacity={0} />
                        </linearGradient>
                    </defs>

                    <XAxis
                        dataKey="recorded_at"
                        tickFormatter={(str) => format(new Date(str), "HH:mm")}
                        axisLine={false}
                        tickLine={false}
                        tick={{ fontSize: 11, fill: "#94a3b8", fontWeight: 500, fontFamily: '"SF Pro Display", sans-serif' }}
                        minTickGap={60}
                        dy={10}
                    />
                    <YAxis
                        axisLine={false}
                        tickLine={false}
                        tick={{ fontSize: 11, fill: "#94a3b8", fontWeight: 500, fontFamily: '"SF Pro Display", sans-serif' }}
                        domain={['auto', 'auto']}
                        allowDataOverflow={false}
                        width={30}
                    />
                    <Tooltip
                        contentStyle={{
                            backgroundColor: "hsl(var(--card))",
                            borderColor: "hsl(var(--border))",
                            borderRadius: "12px",
                            boxShadow: "0 10px 15px -3px rgba(0, 0, 0, 0.1)",
                            padding: "8px 12px",
                        }}
                        itemStyle={{ color: "hsl(var(--foreground))", fontWeight: 700, fontSize: "14px", fontFamily: '"SF Pro Display", sans-serif' }}
                        labelStyle={{ display: "none" }}
                        formatter={(value: number | undefined) => [`${(value ?? 0).toFixed(1)}${unit || ""}`, ""]}
                        cursor={{ stroke: color, strokeWidth: 2, strokeDasharray: "4 4" }}
                        isAnimationActive={false}
                    />
                    <Area
                        type="monotone"
                        dataKey={dataKey}
                        stroke={color}
                        strokeWidth={2.5}
                        fill={`url(#gradient-apple-${dataKey})`}
                        animationDuration={800}
                        animationEasing="ease-out"
                        dot={false}
                        activeDot={{ r: 5, strokeWidth: 3, stroke: "#fff", fill: color }}
                    />
                </AreaChart>
            </ResponsiveContainer>
        </div>
    );
});
