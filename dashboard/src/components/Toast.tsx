import { X, AlertTriangle, CheckCircle, Info } from "lucide-react";
import { useEffect } from "react";
import { cn } from "../lib/utils";

export type ToastType = "success" | "error" | "info" | "warning";

export type ToastData = {
    id: string;
    type: ToastType;
    title: string;
    message?: string;
};

type ToastProps = {
    toast: ToastData;
    onDismiss: (id: string) => void;
    className?: string;
};

const icons = {
    success: CheckCircle,
    error: X,
    info: Info,
    warning: AlertTriangle,
};

const colors = {
    success: "bg-emerald-500/10 border-emerald-500/20 text-emerald-500",
    error: "bg-destructive/10 border-destructive/20 text-destructive",
    info: "bg-blue-500/10 border-blue-500/20 text-blue-500",
    warning: "bg-amber-500/10 border-amber-500/20 text-amber-500",
};

export function Toast({ toast, onDismiss, className }: ToastProps) {
    const Icon = icons[toast.type];

    useEffect(() => {
        const timer = setTimeout(() => {
            onDismiss(toast.id);
        }, 5000);
        return () => clearTimeout(timer);
    }, [toast.id, onDismiss]);

    return (
        <div
            className={cn(
                "flex items-start gap-3 w-full max-w-sm rounded-xl border p-4 shadow-lg backdrop-blur-md animate-in slide-in-from-right-full duration-300",
                colors[toast.type],
                "bg-background/95", // Override strict color with themed background
                className
            )}
        >
            <Icon className="h-5 w-5 mt-0.5 shrink-0" />
            <div className="flex-1 space-y-1">
                <p className="font-semibold text-sm text-foreground">{toast.title}</p>
                {toast.message && <p className="text-sm text-muted-foreground">{toast.message}</p>}
            </div>
            <button
                onClick={() => onDismiss(toast.id)}
                className="text-muted-foreground hover:text-foreground transition-colors"
            >
                <X className="h-4 w-4" />
            </button>
        </div>
    );
}
