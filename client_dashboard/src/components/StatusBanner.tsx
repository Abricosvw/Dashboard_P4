import { EcuData, ConnectionStatus } from '@/types/ecuData';

interface StatusBannerProps {
  ecuData: EcuData | null;
  connectionStatus: ConnectionStatus;
}

export function StatusBanner({ ecuData, connectionStatus }: StatusBannerProps) {
  const getTcuStatus = () => {
    if (!ecuData) return { text: 'Unknown', color: 'text-gray-400' };
    
    if (ecuData.tcuLimpMode) {
      return { text: 'Limp Mode', color: 'text-automotive-danger' };
    }
    
    if (ecuData.tcuProtectionActive) {
      return { text: 'Protection Active', color: 'text-automotive-warning' };
    }
    
    return { text: 'Normal', color: 'text-automotive-success' };
  };

  const getProtectionStatus = () => {
    if (!ecuData) return { text: 'Unknown', color: 'text-gray-400' };
    
    return ecuData.tcuProtectionActive 
      ? { text: 'Active', color: 'text-automotive-warning' }
      : { text: 'Inactive', color: 'text-automotive-success' };
  };

  const tcuStatus = getTcuStatus();
  const protectionStatus = getProtectionStatus();

  return (
    <div className="mb-6 p-4 bg-automotive-card rounded-lg border border-gray-700">
      <div className="flex items-center justify-between flex-wrap gap-4">
        <div className="flex items-center space-x-6">
          <div className="flex items-center space-x-2">
            <span className="text-gray-400">TCU Status:</span>
            <span className={`font-semibold ${tcuStatus.color}`}>
              {tcuStatus.text}
            </span>
          </div>
          
          <div className="flex items-center space-x-2">
            <span className="text-gray-400">Protection:</span>
            <span className={protectionStatus.color}>
              {protectionStatus.text}
            </span>
          </div>
          
          <div className="flex items-center space-x-2">
            <span className="text-gray-400">Data Rate:</span>
            <span className="text-automotive-accent font-mono">
              {connectionStatus.connected ? '20Hz' : '0Hz'}
            </span>
          </div>
        </div>
        
        <div className="text-sm text-gray-400 font-mono">
          Last Update: {ecuData ? new Date().toLocaleTimeString() : 'Never'}
        </div>
      </div>
    </div>
  );
}
