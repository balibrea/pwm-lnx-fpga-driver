library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

-------------------------------------------------------------------------------
-- entity declaration
-------------------------------------------------------------------------------
entity pwm_simple is
  
  generic (
    dc_bits : integer := 16);            -- number of bits for the duty cycle 
  port (
    -- clock & reset signals
    S_AXI_ACLK       : in  std_logic;   -- AXI clock
    S_AXI_ARESETN    : in  std_logic;   -- AXI reset, active low
    -- control input signal 
    duty_cycle       : in  std_logic_vector(31 downto 0);
    -- PWM output 
    pwm              : out std_logic          -- pwn output 
    );        
end entity pwm_simple;

-------------------------------------------------------------------------------
-- architecture
-------------------------------------------------------------------------------
architecture beh of pwm_simple is
begin  
  pwm_pr : process (S_AXI_ACLK, S_AXI_ARESETN) is
     variable counter : unsigned(dc_bits-1 downto 0); -- count clocks tick
  begin  -- process pwm_pr
    if (S_AXI_ARESETN = '0') then
      counter   := (others => '0');
      pwm       <= '0';
    elsif (rising_edge(S_AXI_ACLK)) then
      counter := counter + 1;
      if (counter < unsigned(duty_cycle(dc_bits-1 downto 0))) then
        pwm <= '1';
      else
        pwm <= '0';
      end if;
    end if;
  end process pwm_pr;
end architecture beh;

-------------------------------------------------------------------------------
-- EOF 
-------------------------------------------------------------------------------
