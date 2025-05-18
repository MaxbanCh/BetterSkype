\NeedsTeXFormat{LaTeX2e}%
\ProvidesClass{cs-moi}[2023-09-01, v1.0]%
\AtEndOfClass{\RequirePackage{microtype}}%
\DeclareOption*{\PassOptionsToClass{\CurrentOption}{article}}%
\ProcessOptions*%
\LoadClass{article}%
%==============================
\RequirePackage{unicode-math}
%
\RequirePackage{wasysym}
\RequirePackage{MnSymbol}
%
\RequirePackage[french]{babel}
\RequirePackage{xcolor}
\RequirePackage{colortbl}
\RequirePackage{graphicx,graphbox}
\RequirePackage{multicol}
\RequirePackage{multirow}
\RequirePackage{hhline}
\RequirePackage{float}
\RequirePackage[export]{adjustbox}
\RequirePackage{titlesec}
\RequirePackage{titletoc}
\RequirePackage{geometry}
\RequirePackage[nocheck]{fancyhdr}
\RequirePackage[inline]{enumitem}
\RequirePackage[linesnumbered, ruled, vlined, french]{algorithm2e}
%
\RequirePackage{tcolorbox}
\tcbuselibrary{skins, theorems, breakable, listings}
%
\RequirePackage{sidenotes}
\RequirePackage{subcaption}
\RequirePackage{todonotes}
%==============================
\definecolor{maincolor}{rgb}{0.15706, 0.24725, 0.40667} %choisir sa couleur
\definecolor{solcolor}{cmyk}{0,0,0,1}
\colorlet{lightmaincolor}{maincolor!30!white}
\colorlet{verylightmaincolor}{maincolor!10!white}
\colorlet{backmaincolor}{maincolor!0!white}
\colorlet{darkmaincolor}{maincolor!90!black}
\colorlet{deepmaincolor}{maincolor!50!black}
%==============================
%==============================
% caractères spéciaux
\lstset{
  basicstyle=\linespread{1}\ttfamily,
  commentstyle=\color{maincolor},
  showstringspaces=false,
  breaklines=true,
  numbersep=4mm,
  tabsize=2,
  language=C,
}
%
\lstnewenvironment{ocaml}{\lstset{language=caml}}{}
\lstnewenvironment{python}{\lstset{language=python}}{}
\lstnewenvironment{C}{\lstset{language=C}}{}
\lstnewenvironment{R}{\lstset{language=R}}{}
\newcommand{\code}[1]{\lstinline[basicstyle=\ttfamily, breaklines]{#1}}
\newcommand{\codec}[1]{\text{\textcolor{maincolor}{\code{#1}}}}
%==============================
%==============================
% géométrie
\geometry{%
  hoffset=0mm, 
  voffset=0mm,%
  paperwidth=210mm, 
  left=25mm, 
  right=25mm,% width = paperwidth - left - right
  paperheight=297mm, 
  top=14mm, 
  bottom=14mm,% height = paperheight - top - bottom
  % marginparsep=0mm, marginparwidth=0mm,% textwidth = width - marginparsep - marginparwidth
  headheight=5mm, 
  headsep=5mm, 
  footskip=10mm, 
  includehead, 
  includefoot,% textheight = height - headheight - headsep - footskip si les options includehead et includefoot sont présentes
}%
%==============================
% entête et pied de page
%-------------------------
\pagestyle{fancy}
% \setlength{\parindent}{0mm}
\renewcommand{\headrulewidth}{.5pt}
\renewcommand{\footrulewidth}{.5pt}
\renewcommand{\headrule}{{\color{maincolor}\hrule width\headwidth height\headrulewidth \vskip-\headrulewidth}}
\renewcommand{\footrule}{{\color{maincolor}\vskip-\footruleskip\vskip-\footrulewidth\hrule width\headwidth height\footrulewidth\vskip\footruleskip}}
%-------------------------
\newcommand{\uleft}[1]{\lhead{\rmfamily\textcolor{maincolor}{#1}}}
\newcommand{\ucent}[1]{\chead{\rmfamily\textcolor{maincolor}{#1}}}
\newcommand{\uright}[1]{\rhead{\rmfamily\textcolor{maincolor}{#1}}}
\newcommand{\dleft}[1]{\lfoot{\rmfamily\textcolor{maincolor}{#1}}}
\newcommand{\dcent}[1]{\cfoot{\rmfamily\textcolor{maincolor}{#1}}}
\newcommand{\dright}[1]{\rfoot{\rmfamily\textcolor{maincolor}{#1}}}
\uleft{}
\ucent{}
\uright{}
%==============================
\setcounter{tocdepth}{2}
\newcommand{\marginsecnumber}[1]{\makebox[0pt][r]{#1\hspace{1ex}}}
%==============================
% puces des listes
\setlist{font=\color{maincolor}\rmfamily\bfseries}
\setlist[itemize,1]{label=$\filleddiamond$}
\setlist[itemize,2]{label=$\smalldiamond$}
\setlist[itemize,3]{label=$\diamonddot$}
%==============================
% title
%\renewcommand{\maketitle}[1]{\noindent\textbf{\color{maincolor}\rmfamily\Huge #1}\par\bigskip}
\renewcommand{\maketitle}[1]{\begin{center}
  {\noindent\textbf{\color{maincolor}\huge\@title}\par\bigskip}
  {\noindent\@author\Large\par\bigskip}
  {\noindent\@date\large\par\bigskip}
\end{center}}
%-------------------------
% section
\titleformat{\section}{\Large\rmfamily\bfseries\color{maincolor}}{\marginsecnumber\thesection}{0pt}{}
\titlespacing{\section}{0pt}{*2.5}{.5ex plus .5ex}
%-------------------------
% subsection
\titleformat{\subsection}{\large\rmfamily\bfseries\color{maincolor}}{\marginsecnumber\thesubsection}{0pt}{}
\titlespacing{\subsection}{0pt}{*1.5}{.5ex plus .5ex}
%-------------------------
% subsubsection
\titleformat{\subsubsection}{\large\rmfamily\bfseries\color{maincolor}}{\marginsecnumber\thesubsubsection}{0pt}{}
\titlespacing{\subsubsection}{0pt}{*1.5}{.5ex plus .5ex}
%-------------------------
% paragraphs
\renewenvironment{paragraph}[1]%
{\par\smallskip\noindent\textcolor{maincolor}{\bfseries #1.}\xspace}%
{\hfill$\medtriangleleft$\par\medskip}
%
\newenvironment{exemp}%
{\par\smallskip\noindent\textcolor{maincolor}{\bfseries Exemple}\xspace}%
{\hfill$\medtriangleleft$\par\medskip}
%
\newtheorem{exempinl}{\color{maincolor}Exemple}
\renewcommand{\theexempinl}{\arabic{exempinl}.}
%
\newtheorem{definline}{\color{maincolor}Définition}
\renewcommand{\thedefinline}{\arabic{definline}.}
%-------------------------
\newcounter{exo}
\newcounter{quest}
\newcounter{squest}
\newcounter{ssquest}
\setcounter{exo}{0}
\setcounter{quest}{0}
\setcounter{quest}{0}
\setcounter{quest}{0}
% exercices
\titleclass{\exo}{straight}[\section]
\titleformat{\exo}[block]{\textcolor{maincolor}{\rmfamily\bfseries{\huge Exercice \theexo}}}{}{0pt}{}
\titlespacing{\exo}{0pt}{2.0ex plus .1ex minus .2ex}{1.0ex plus .1ex minus .2ex}
\newcommand{\exerc}{\setcounter{quest}{0}\exo{}}
% question
\newcommand{\question}{%
  \stepcounter{quest}%
  \setcounter{squest}{0}%
  \setlength{\leftskip}{0em}%
  \par\medskip\noindent%
  \textbf{Question \thequest.\hspace*{2\wordsep}}%
}
% sous-question
\newcommand{\squestion}{%
  \stepcounter{squest}%
  \setcounter{ssquest}{0}%
  % \setlength{\leftskip}{1em}%
  \par\noindent%
  \textbf{\textcolor{maincolor}{$\Box$} \thequest.\thesquest.\hspace*{2\wordsep}}%
}
% sous-sous-question
\newcommand{\ssquestion}{%
  \stepcounter{ssquest}%
  % \setlength{\leftskip}{2em}%
  \par\noindent%
  \textbf{\textcolor{maincolor}{$\rightslice$} \thequest.\thesquest.\thessquest.\hspace*{2\wordsep}}%
}
%==============================
% environnements
% définition
\newtcbtheorem[auto counter]{tcbdefn}{\faDownload\enspace Définition}{%
  enhanced,
  breakable,
  colframe=verylightmaincolor,
  colback=backmaincolor,
  top=4mm,
  fonttitle={\rmfamily\bfseries\color{black}},
  separator sign dash,
  attach boxed title to top left={yshift=-3mm},
  boxed title style={colframe=verylightmaincolor, colback=verylightmaincolor},
  %% enhanced,
  breakable,
  %% colframe=lightmaincolor,colback=black,colbacktitle=white,
  %% fonttitle={\rmfamily\bfseries\color{darkmaincolor}},
  %% separator sign dash,
}{defn}
\newenvironment{defn}[2][]{\begin{tcbdefn}{#2}{#1}}{\end{tcbdefn}}
% théorème
\newtcbtheorem[auto counter]{tcbthm}{\faCog\enspace Théorème}{%
  enhanced,
  breakable,
  colframe=verylightmaincolor,
  colback=backmaincolor,
  top=4mm,
  fonttitle={\rmfamily\bfseries\color{black}},
  separator sign dash,
  attach boxed title to top left={yshift=-3mm},
  boxed title style={colframe=verylightmaincolor, colback=verylightmaincolor},
}{thm}
\newenvironment{thm}[2][]{\begin{tcbthm}{#2}{#1}}{\end{tcbthm}}
% proposition
\newtcbtheorem[auto counter]{tcbprop}{Proposition}{%
  enhanced,
  breakable,
  colframe=verylightmaincolor,
  colback=backmaincolor,
  top=4mm,
  fonttitle={\rmfamily\bfseries\color{black}},
  separator sign dash,
  attach boxed title to top left={yshift=-3mm},
  boxed title style={colframe=verylightmaincolor, colback=verylightmaincolor},
}{prop}
\newenvironment{prop}[2][]{\begin{tcbprop}{#2}{#1}}{\end{tcbprop}}
% lemme
\newtcbtheorem[auto counter]{tcblem}{Lemme}{%
  enhanced,
  breakable,
  colframe=verylightmaincolor,
  colback=backmaincolor,
  top=4mm,
  fonttitle={\rmfamily\bfseries\color{black}},
  separator sign dash,
  attach boxed title to top left={yshift=-3mm},
  boxed title style={colframe=verylightmaincolor, colback=verylightmaincolor},
}{thm}
\newenvironment{lem}[2][]{\begin{tcblem}{#2}{#1}}{\end{tcblem}}
% corolaire
\newtcbtheorem[auto counter]{tcbcor}{Corolaire}{%
  enhanced,
  breakable,
  colframe=verylightmaincolor,
  colback=backmaincolor,
  top=4mm,
  fonttitle={\rmfamily\bfseries\color{black}},
  separator sign dash,
  attach boxed title to top left={yshift=-3mm},
  boxed title style={colframe=verylightmaincolor, colback=verylightmaincolor},
}{thm}
\newenvironment{cor}[2][]{\begin{tcbcor}{#2}{#1}}{\end{tcbcor}}
% exemple
\newtcbtheorem[no counter]{tcbexemple}{Exemple}{%
  enhanced,
  breakable,
  colframe=verylightmaincolor,
  colback=backmaincolor,
  top=4mm,
  fonttitle={\rmfamily\bfseries\color{black}},
  separator sign dash,
  attach boxed title to top left={yshift=-3mm},
  boxed title style={colframe=verylightmaincolor, colback=verylightmaincolor},
}{}
\newenvironment{exemple}[2][]{\begin{tcbexemple}{#2}{#1}}{\end{tcbexemple}}
% démonstration
\newenvironment{demo}%
  {\noindent\emph{Démonstration.}\xspace}%
  {\hfill$\Box$\par\medskip}
% remarque
\newtcbtheorem[no counter]{tcbremarque}{Remarque}{%
  enhanced,
  breakable,
  colframe=verylightmaincolor,
  colback=backmaincolor,
  top=4mm,
  fonttitle={\rmfamily\bfseries\color{black}},
  separator sign dash,
  attach boxed title to top left={yshift=-3mm},
  boxed title style={colframe=verylightmaincolor, colback=verylightmaincolor},
}{}
\newenvironment{remarque}[2][]{\begin{tcbremarque}{#2}{#1}}{\end{tcbremarque}}
%==============================
% algorithmes
\DontPrintSemicolon
\SetKwInput{Ent}{Entrée}
\SetKwInput{Out}{Sortie}
\SetKwProg{Fn}{fonction}{}{}
\SetKwIF{If}{ElseIf}{Else}{si}{alors}{sinon si}{sinon}{fin si}
\SetKwFor{While}{pour}{faire}{}%
\SetKwFor{Repeat}{tant que}{faire}{}%
\SetKw{Ret}{renvoyer}%
%==============================