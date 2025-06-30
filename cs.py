import numpy as np
import scipy.signal as signal
import matplotlib.pyplot as plt
import sympy as sp
import control

def confirm_input(message):
    """Ask the user for confirmation before proceeding."""
    while True:
        confirm = input(f"{message} (y/n): ").strip().lower()
        if confirm in ['y', 'yes']:
            return True
        elif confirm in ['n', 'no']:
            return False
        else:
            print("Invalid input! Please enter 'y' or 'n'.")
def create_transfer_function():
    """Create a transfer function from user input, either via numerator/denominator or state-space representation."""
    while True:
        choice = input("\nChoose input method:\n1. Transfer Function (Numerator/Denominator)\n2. State-Space Model\nEnter 1 or 2: ")
        
        if choice == '1':
            k = input("Enter gain k (default is 1): ") or "1"
            k = sp.symbols('k') if k == 'k' else float(k)

            n = int(input("Enter number of factors in numerator: "))
            numerator = [sp.sympify(input(f"Enter numerator factor {i+1} (e.g., 's+1'): ")) for i in range(n)]
            
            d = int(input("Enter number of factors in denominator: "))
            denominator = [sp.sympify(input(f"Enter denominator factor {i+1} (e.g., 's+2'): ")) for i in range(d)]

            s = sp.symbols('s')
            num_expr = k * sp.prod(numerator)
            den_expr = sp.prod(denominator)

            num_coeffs = sp.Poly(num_expr, s).all_coeffs()
            den_coeffs = sp.Poly(den_expr, s).all_coeffs()

            num_coeffs = [float(c) if c != sp.symbols('k') else 1 for c in num_coeffs]
            den_coeffs = [float(c) for c in den_coeffs]

            print("\nYou entered the following transfer function:")
            print(f"Symbolic Transfer Function:")
            display_expr = sp.simplify(num_expr / den_expr)
            sp.pprint(display_expr, use_unicode=True)

            print(f"\nNumerator Coefficients: {num_coeffs}")
            1
            print(f"Denominator Coefficients: {den_coeffs}")


            if confirm_input("Proceed with this transfer function?"):
                return control.TransferFunction(num_coeffs, den_coeffs)

        elif choice == '2':
            print("\nEnter State-Space Matrices:")
            try:
                A = np.array(eval(input("Enter A matrix (as a list of lists): ")))
                B = np.array(eval(input("Enter B matrix (as a list): ")))
                C = np.array(eval(input("Enter C matrix (as a list): ")))
                D = np.array(eval(input("Enter D matrix (as a single value or list): ")))

                print("\nYou entered the following state-space representation:")
                print(f"A matrix:\n{A}")
                print(f"B matrix:\n{B}")
                print(f"C matrix:\n{C}")
                print(f"D matrix:\n{D}")

                if confirm_input("Proceed with this state-space model?"):
                    return control.ss2tf(A, B, C, D)
            except Exception as e:
                print("Invalid input! Please enter valid matrices.")
                print(f"Error: {e}")

        print("\nInvalid choice or user canceled. Please try again.")


def step_response(sys):
    t, y = control.step_response(sys)
    plt.figure()
    plt.plot(t, y)
    plt.title('Step Response')
    plt.xlabel('Time (s)')
    plt.ylabel('Output')
    plt.grid(True)
    plt.show()

def impulse_response(sys):
    t, y = control.impulse_response(sys)
    plt.figure()
    plt.plot(t, y)
    plt.title('Impulse Response')
    plt.xlabel('Time (s)')
    plt.ylabel('Output')
    plt.grid(True)
    plt.show()

def stability_analysis(sys):
    poles = control.poles(sys)
    print("\nPoles of the system:", poles)
    if all(np.real(p) < 0 for p in poles):
        print("The system is STABLE.")
    else:
        print("The system is UNSTABLE.")

def bode_plot(sys):
    

    # Find poles and zeros
    poles = control.poles(sys)
    zeros = control.zeros(sys)

    # Calculate important frequencies (only real parts)
    important_freqs = []
    for p in poles:
        if np.isfinite(p.real) and p.real != 0:
            important_freqs.append(abs(p.real))
    for z in zeros:
        if np.isfinite(z.real) and z.real != 0:
            important_freqs.append(abs(z.real))

    if important_freqs:
        min_freq = min(important_freqs)
        max_freq = max(important_freqs)
    else:
        min_freq = 1  # Default safe values
        max_freq = 10

    low = min_freq / 10
    high = max_freq * 10

    # Frequency range (log spaced)
    omega = np.logspace(np.log10(low), np.log10(high), 500)

    # Now plot
    control.bode_plot(sys, dB=True)
    plt.show()


    # Margins
    gm, pm, wg, wp = control.margin(sys)
    print(f"\nGain Margin (GM): {gm:.2f}")
    print(f"Phase Margin (PM): {pm:.2f} degrees")
    print(f"Gain Crossover Frequency (Wg): {wg:.2f} rad/s")
    print(f"Phase Crossover Frequency (Wp): {wp:.2f} rad/s")

    # Stability Analysis based on Poles
    real_parts = [p.real for p in poles]
    if all(r < 0 for r in real_parts):
        stability = "Stable"
    elif any(r > 0 for r in real_parts):
        stability = "Unstable"
    else:
        stability = "Marginally Stable"
    print(f"\nSystem Stability: {stability}")

    # Damping Analysis based on ζ (zeta)
    for idx, p in enumerate(poles, 1):
        if np.iscomplex(p):
            wn = abs(p)  # Natural frequency
            zeta = -p.real / wn  # Damping ratio ζ = -σ/ωn

            print(f"\nPole {idx}: {p}")
            print(f"  Damping Ratio (ζ): {zeta:.2f}")

            if zeta > 1:
                print("  Nature: Overdamped")
            elif np.isclose(zeta, 1, atol=0.05):
                print("  Nature: Critically Damped")
            elif 0 < zeta < 1:
                print("  Nature: Underdamped")
            elif np.isclose(zeta, 0, atol=0.05):
                print("  Nature: Undamped (Pure Oscillation)")
            else:
                print("  Nature: Unstable")

    print("\nFormula Used for Damping Ratio:")
    print("ζ = - (Real Part of Pole) / (Magnitude of Pole)\n")


def root_locus_details(sys):
    print("\n------ ROOT LOCUS ANALYSIS ------\n")
    poles, zeros = sys.poles(), sys.zeros()
    
    print(f"Poles: {poles}")
    print(f"Zeros: {zeros}")
    
    plt.figure(figsize=(8,6))
    control.root_locus(sys, grid=True)
    plt.title("Root Locus")
    plt.xlabel("Real Axis")
    plt.ylabel("Imaginary Axis")
    plt.grid(True)
    plt.show()
def get_natural_frequency_and_damping(sys):
    poles = control.poles(sys)
    for p in poles:
        if np.iscomplex(p) and np.real(p) < 0:
            wn = np.abs(p)
            zeta = -np.real(p) / wn
            return wn, zeta, p
    return None, None, None

def time_domain_specifications(sys):
    wn, zeta, _ = get_natural_frequency_and_damping(sys)
    print("\nTime-Domain Specifications:")

    if wn is not None and zeta is not None and 0 < zeta < 1:
        # Analytical formulas for underdamped systems
        Mp = np.exp(-np.pi * zeta / np.sqrt(1 - zeta ** 2)) * 100
        Tp = np.pi / (wn * np.sqrt(1 - zeta ** 2))
        Ts = 4 / (zeta * wn)
        Tr = (1.8 / wn) if 0.5 < zeta < 0.9 else None

        print(f"Method: Analytical (Formulas)")
        print(f"Peak Overshoot (Mp): {Mp:.2f}%")
        print(f"Peak Time (Tp): {Tp:.4f} seconds")
        print(f"Settling Time (Ts): {Ts:.4f} seconds")
        print(f"Rise Time (Tr): {Tr:.4f} seconds" if Tr else "Rise Time not available analytically.")
    else:
        # Fallback to numerical method
        print("Method: Numerical (Step Response)")
        t, y = control.step_response(sys)

        Mp = (np.max(y) - 1) * 100
        Tp = t[np.argmax(y)]

        within_bounds = np.where(np.abs(y - 1) <= 0.02)[0]
        Ts = t[within_bounds[-1]] if len(within_bounds) > 0 else None

        rise_indices = np.where((y >= 0.1) & (y <= 0.9))[0]
        Tr = t[rise_indices[-1]] - t[rise_indices[0]] if len(rise_indices) > 0 else None

        print(f"Peak Overshoot (Mp): {Mp:.2f}%")
        print(f"Peak Time (Tp): {Tp:.4f} seconds")
        print(f"Settling Time (Ts): {Ts:.4f} seconds" if Ts else "Settling Time not found.")
        print(f"Rise Time (Tr): {Tr:.4f} seconds" if Tr else "Rise Time not found.")



def menu():
    sys = create_transfer_function()
    while True:
        print("\n----------------------------")
        print("        MAIN MENU")
        print("----------------------------")
        print("1. Step Response")
        print("2. Impulse Response")
        print("3. Stability Analysis")
        print("4. Time Domain Specifications")
        print("5. Bode Plot")
        print("6. Root Locus Plot")
        print("7. Convert to State-Space")
        print("8. Exit")
        choice = input("\nEnter your choice: ")

        if choice == '1':
            step_response(sys)
        elif choice == '2':
            impulse_response(sys)
        elif choice == '3':
            stability_analysis(sys)
        elif choice == '4':
            time_domain_specifications(sys)
        elif choice == '5':
            bode_plot(sys)
        elif choice == '6':
            root_locus_details(sys)
        elif choice == '7':
            ss_sys = control.tf2ss(sys)
            print("\nState-Space Representation:")
            print(f"A matrix:\n{ss_sys.A}")
            print(f"B matrix:\n{ss_sys.B}")
            print(f"C matrix:\n{ss_sys.C}")
            print(f"D matrix:\n{ss_sys.D}")
        elif choice == '8':
            print("Exiting program. Goodbye!")
            break
        else:
            print("Invalid choice! Please enter a number between 1-8.")

# Run the program
menu()