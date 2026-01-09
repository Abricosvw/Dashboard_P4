import { useEffect, useRef } from 'react';
import { GaugeConfig } from '@/types/ecuData';
import { GaugeDisplaySettings } from './GaugeSettings';

interface GaugeProps {
  config: GaugeConfig;
  settings: GaugeDisplaySettings;
  style?: React.CSSProperties;
  isDragging?: boolean;
}

export function Gauge({ config, settings, style, isDragging }: GaugeProps) {
  const canvasRef = useRef<HTMLCanvasElement>(null);

  const getSizeDimensions = () => {
    switch (settings.size) {
      case 'small': return { width: 150, height: 150, radius: 50, fontSize: 'text-lg', padding: 'p-3' };
      case 'medium': return { width: 200, height: 200, radius: 70, fontSize: 'text-2xl', padding: 'p-4' };
      case 'large': return { width: 280, height: 280, radius: 100, fontSize: 'text-3xl', padding: 'p-6' };
      case 'xlarge': return { width: 350, height: 350, radius: 130, fontSize: 'text-4xl', padding: 'p-8' };
      default: return { width: 200, height: 200, radius: 70, fontSize: 'text-2xl', padding: 'p-4' };
    }
  };

  const dimensions = getSizeDimensions();

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;

    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    const centerX = canvas.width / 2;
    const centerY = canvas.height / 2;
    const radius = dimensions.radius;

    // Clear canvas
    ctx.clearRect(0, 0, canvas.width, canvas.height);

    const normalizedValue = Math.max(0, Math.min(1, (config.value - config.min) / (config.max - config.min)));

    // Determine color based on thresholds
    let strokeColor = config.color;
    if (config.dangerThreshold && config.value >= config.dangerThreshold) {
      strokeColor = '#FF3366';
    } else if (config.warningThreshold && config.value >= config.warningThreshold) {
      strokeColor = '#FF6B35';
    }

    if (settings.gaugeStyle === 'linear') {
      // Linear bar style
      const barHeight = 20;
      const barWidth = radius * 2 - 40;
      const barX = centerX - barWidth / 2;
      const barY = centerY - barHeight / 2;

      // Background bar
      ctx.fillStyle = '#333';
      ctx.fillRect(barX, barY, barWidth, barHeight);

      // Value bar
      ctx.fillStyle = strokeColor;
      ctx.fillRect(barX, barY, barWidth * normalizedValue, barHeight);

      // Target indicator
      if (config.target !== undefined && settings.showTargets) {
        const targetNormalized = Math.max(0, Math.min(1, (config.target - config.min) / (config.max - config.min)));
        const targetX = barX + barWidth * targetNormalized;
        
        ctx.strokeStyle = '#FFD700';
        ctx.lineWidth = 3;
        ctx.beginPath();
        ctx.moveTo(targetX, barY - 5);
        ctx.lineTo(targetX, barY + barHeight + 5);
        ctx.stroke();
      }
    } else {
      // Arc styles
      const startAngle = settings.gaugeStyle === 'arc' ? Math.PI : Math.PI * 0.75;
      const endAngleBase = settings.gaugeStyle === 'arc' ? Math.PI * 2 : Math.PI * 2.25;
      const totalAngle = endAngleBase - startAngle;

      // Background arc
      ctx.beginPath();
      ctx.arc(centerX, centerY, radius, startAngle, endAngleBase);
      ctx.strokeStyle = '#333';
      ctx.lineWidth = settings.size === 'small' ? 6 : settings.size === 'xlarge' ? 12 : 8;
      ctx.stroke();

      // Value arc
      const endAngle = startAngle + (totalAngle * normalizedValue);

      ctx.beginPath();
      ctx.arc(centerX, centerY, radius, startAngle, endAngle);
      ctx.strokeStyle = strokeColor;
      ctx.lineWidth = settings.size === 'small' ? 6 : settings.size === 'xlarge' ? 12 : 8;
      ctx.lineCap = 'round';
      ctx.stroke();

      // Target indicator
      if (config.target !== undefined && settings.showTargets) {
        const targetNormalized = Math.max(0, Math.min(1, (config.target - config.min) / (config.max - config.min)));
        const targetAngle = startAngle + (totalAngle * targetNormalized);
        
        ctx.beginPath();
        ctx.arc(centerX, centerY, radius + 5, targetAngle - 0.05, targetAngle + 0.05);
        ctx.strokeStyle = '#FFD700';
        ctx.lineWidth = 4;
        ctx.stroke();
      }

      // Tick marks
      if (!settings.compactMode) {
        ctx.strokeStyle = '#666';
        ctx.lineWidth = 2;
        const tickCount = settings.size === 'small' ? 5 : 10;
        for (let i = 0; i <= tickCount; i++) {
          const angle = startAngle + (totalAngle * i / tickCount);
          const startRadius = radius - (settings.size === 'small' ? 8 : 10);
          const endRadius = radius - (settings.size === 'small' ? 5 : 5);

          ctx.beginPath();
          ctx.moveTo(
            centerX + Math.cos(angle) * startRadius,
            centerY + Math.sin(angle) * startRadius
          );
          ctx.lineTo(
            centerX + Math.cos(angle) * endRadius,
            centerY + Math.sin(angle) * endRadius
          );
          ctx.stroke();
        }
      }
    }
  }, [config, settings, dimensions]);

  const containerClass = `gauge-container rounded-xl ${dimensions.padding} ${
    isDragging ? 'opacity-50 scale-95' : ''
  } transition-all duration-200`;

  return (
    <div 
      className={containerClass}
      style={style}
    >
      {settings.showTitles && !settings.compactMode && (
        <div className="text-center mb-4">
          <h3 className={`${settings.size === 'small' ? 'text-sm' : settings.size === 'xlarge' ? 'text-xl' : 'text-lg'} font-orbitron font-bold text-white`}>
            {config.title}
          </h3>
          <p className={`${settings.size === 'small' ? 'text-xs' : 'text-sm'} text-gray-400`}>
            {config.subtitle}
          </p>
        </div>
      )}
      
      <div 
        className="relative mx-auto mb-4"
        style={{ width: dimensions.width, height: dimensions.height }}
      >
        <canvas 
          ref={canvasRef}
          width={dimensions.width} 
          height={dimensions.height} 
          className="absolute inset-0"
        />
        {settings.showValues && (
          <div className="absolute inset-0 flex items-center justify-center">
            <div className="text-center">
              <div className={`${dimensions.fontSize} font-orbitron font-bold text-automotive-accent`}>
                {config.value.toFixed(config.unit === 'RPM' ? 0 : 1)}
              </div>
              {!settings.compactMode && (
                <div className={`${settings.size === 'small' ? 'text-xs' : 'text-sm'} text-gray-400`}>
                  {config.unit}
                </div>
              )}
            </div>
          </div>
        )}
      </div>
      
      {!settings.compactMode && (
        <div className="text-center space-y-1">
          {config.target !== undefined && settings.showTargets && (
            <div className={`${settings.size === 'small' ? 'text-xs' : 'text-sm'} text-gray-400`}>
              Target: <span className="text-automotive-yellow">{config.target.toFixed(1)}</span> {config.unit}
            </div>
          )}
          <div className={`${settings.size === 'small' ? 'text-xs' : 'text-xs'} text-gray-500`}>
            Range: {config.min}-{config.max} {config.unit}
          </div>
        </div>
      )}
    </div>
  );
}
